#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include "TourOverlay.h"
#include "ProjectInfo.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace
{
    std::wstring wide(const std::string& text)
    {
        if(text.empty())return {};
        int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),nullptr,0);
        if(!count)throw std::runtime_error("Overlay metadata is not valid UTF-8 (ProjectInfo.h)");
        std::wstring result(count,L' ');
        MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),result.data(),count);return result;
    }
    struct Surface {
        HDC dc=nullptr;HBITMAP bitmap=nullptr;HGDIOBJ oldBitmap=nullptr,oldFont=nullptr;
        std::array<HFONT,4> fonts{};
        ~Surface(){if(oldFont)SelectObject(dc,oldFont);if(oldBitmap)SelectObject(dc,oldBitmap);
            for(auto font:fonts)if(font)DeleteObject(font);if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);}
    };
    GLuint compile(GLenum type,const char* source)
    {
        GLuint shader=glCreateShader(type);glShaderSource(shader,1,&source,nullptr);glCompileShader(shader);
        GLint okay=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&okay);
        if(!okay){char log[4096]{};glGetShaderInfoLog(shader,4096,nullptr,log);glDeleteShader(shader);throw std::runtime_error(std::string("Tour overlay shader: ")+log);}return shader;
    }
    void enable(GLenum cap,GLboolean value){if(value)glEnable(cap);else glDisable(cap);}
}
TourOverlay::TourOverlay()
{
    const char* vertex=R"GLSL(#version 330 core
uniform vec4 rectangle;uniform vec2 viewportSize;
out vec2 uv;
void main(){const vec2 p[6]=vec2[6](vec2(0,0),vec2(1,0),vec2(0,1),vec2(1,0),vec2(1,1),vec2(0,1));
 uv=p[gl_VertexID];vec2 pixel=rectangle.xy+uv*rectangle.zw;
 gl_Position=vec4(pixel.x/viewportSize.x*2.0-1.0,1.0-pixel.y/viewportSize.y*2.0,0,1);})GLSL";
    const char* fragment=R"GLSL(#version 330 core
uniform sampler2D panel;in vec2 uv;out vec4 fragColor;
void main(){fragColor=texture(panel,uv);})GLSL";
    GLuint vs=compile(GL_VERTEX_SHADER,vertex),fs=0;
    try{fs=compile(GL_FRAGMENT_SHADER,fragment);}catch(...){glDeleteShader(vs);throw;}
    program=glCreateProgram();glAttachShader(program,vs);glAttachShader(program,fs);glLinkProgram(program);
    glDeleteShader(vs);glDeleteShader(fs);GLint okay=0;glGetProgramiv(program,GL_LINK_STATUS,&okay);
    if(!okay){char log[4096]{};glGetProgramInfoLog(program,4096,nullptr,log);glDeleteProgram(program);program=0;throw std::runtime_error(std::string("Tour overlay link: ")+log);}
    rectangleLocation=glGetUniformLocation(program,"rectangle");viewportLocation=glGetUniformLocation(program,"viewportSize");
    // Samplers default to texture unit zero; no global texture/program changes.
    glGenVertexArrays(1,&vao);glGenTextures(1,&texture);
}
TourOverlay::~TourOverlay(){glDeleteTextures(1,&texture);glDeleteVertexArrays(1,&vao);glDeleteProgram(program);}

void TourOverlay::rasterize(bool help,float scale,const TourControls& controls,const Status& status)
{
    int width=std::max(1,int(std::round((help?940:700)*scale))),height=std::max(1,int(std::round((help?520:154)*scale)));
    Surface surface;surface.dc=CreateCompatibleDC(nullptr);if(!surface.dc)throw std::runtime_error("Cannot create tour-overlay text DC");
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-height;
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    void* raw=nullptr;surface.bitmap=CreateDIBSection(surface.dc,&info,DIB_RGB_COLORS,&raw,nullptr,0);
    if(!surface.bitmap||!raw)throw std::runtime_error("Cannot allocate tour-overlay text bitmap");
    surface.oldBitmap=SelectObject(surface.dc,surface.bitmap);
    const int sizes[]={help?26:20,18,16,14};
    for(int i=0;i<4;++i){surface.fonts[i]=CreateFontW(-std::max(10,int(std::round(sizes[i]*scale))),0,0,0,i==0?FW_SEMIBOLD:FW_NORMAL,
        FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Segoe UI");
        if(!surface.fonts[i])throw std::runtime_error("Cannot create tour-overlay font");}
    surface.oldFont=SelectObject(surface.dc,surface.fonts[0]);
    PatBlt(surface.dc,0,0,width,height,BLACKNESS);SetBkMode(surface.dc,TRANSPARENT);SetTextColor(surface.dc,RGB(255,255,255));
    auto text=[&](float x,float y,float w,float h,int font,const std::string& label,bool wrap=false){auto value=wide(label);if(value.empty())return;
        SelectObject(surface.dc,surface.fonts[font]);RECT r{int(x*scale),int(y*scale),int((x+w)*scale),int((y+h)*scale)};
        if(!DrawTextW(surface.dc,value.c_str(),int(value.size()),&r,DT_LEFT|DT_NOPREFIX|(wrap?DT_WORDBREAK:DT_SINGLELINE)))
            throw std::runtime_error("Cannot rasterize tour-overlay text");};
    char line[256]{};
    text(14,10,help?910:330,36,0,ProjectInfo::title);
    if(!help){std::snprintf(line,sizeof(line),"%s %.0f m/s | %s",status.movement,status.speed,controls.paused?"PAUSED":"RUNNING");text(355,14,330,24,2,line);}
    text(14,help?49:40,help?910:672,40,2,ProjectInfo::team,true);
    std::snprintf(line,sizeof(line),"PRIMARY %s: %s | COLOR %s | FXAA %s",ProjectInfo::primaryEffect,status.shadows?"ON":"OFF",status.color?"ON":"OFF",status.fxaa?"ON":"OFF");
    text(14,help?119:82,help?910:672,22,2,line);
    std::snprintf(line,sizeof(line),"SKY %s | SUN AZ %.0f deg | [ ] rotate / R reset",status.sky,status.sunAzimuth);text(14,help?139:105,help?910:672,22,3,line);
    if(!help)text(14,128,672,22,3,"F1 / H HELP   |   F2 HIDE UI   |   P PAUSE ANIMATION");
    else{
        std::snprintf(line,sizeof(line),"%s %.0f m/s | ANIMATION %s | SUN ELEVATION 50 deg",status.movement,status.speed,controls.paused?"PAUSED":"RUNNING");text(14,96,910,22,2,line);
        const char* left[]={"CAMERA & MOVEMENT","W/A/S/D Move | Mouse Look","F Walk / Flight","Shift Sprint | G Fast flight","Space / Left Ctrl Up / Down (flight)",
            "LANDMARK SHORTCUTS","1 Baiterek | 2 Talan Towers","3 Khan Shatyr | 5 Ak Orda","6 Botanical Garden | 7 EXPO / Nur Alem","8 MEGA Silk Way","9 Esil River / Atyrau Bridge"};
        const char* right[]={"GRAPHICS & TIME","F6 Sun shadows","F7 Color grading","F8 FXAA","F9 Cinematic / Anime sky","P Pause / resume water & fountains",
            "[ / ] Sun azimuth -/+15 deg per press","R Reset default sunlight","F1 / H Toggle help","F2 Hide / show all overlays","ESC Exit tour"};
        for(int i=0;i<11;++i){text(18,174+i*24,440,23,i==0||i==5?1:2,left[i]);text(494,174+i*24,430,23,i==0?1:2,right[i]);}
        text(18,452,906,40,2,std::string("TOPIC: ")+ProjectInfo::topic,true);
        text(18,497,906,20,3,"Camera stays active while water / fountain animation is paused. This is a 3D tour.");
    }
    GdiFlush();auto* source=static_cast<unsigned char*>(raw);std::vector<unsigned char> rgba(std::size_t(width)*height*4);
    for(int y=0;y<height;++y)for(int x=0;x<width;++x){std::size_t i=(std::size_t(y)*width+x)*4;
        float coverage=std::max({source[i],source[i+1],source[i+2]})/255.f;
        bool border=x<1||y<1||x>=width-1||y>=height-1;float backgroundAlpha=border?.92f:.80f;
        const float background[3]={border?.20f:.035f,border?.34f:.055f,border?.43f:.08f};
        float alpha=coverage+(1-coverage)*backgroundAlpha;
        for(int c=0;c<3;++c){float premultiplied=coverage+(1-coverage)*background[c]*backgroundAlpha;rgba[i+c]=static_cast<unsigned char>(premultiplied/alpha*255+.5f);}
        rgba[i+3]=static_cast<unsigned char>(alpha*255+.5f);
    }
    glBindTexture(GL_TEXTURE_2D,texture);
    if(width!=textureWidth||height!=textureHeight){glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());textureWidth=width;textureHeight=height;}
    else glTexSubImage2D(GL_TEXTURE_2D,0,0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);++updates;
}

void TourOverlay::render(int width,int height,const TourControls& controls,const Status& status)
{
    draws=0;if(!controls.overlayVisible||width<=0||height<=0)return;
    float logicalWidth=controls.helpVisible?940.f:700.f,logicalHeight=controls.helpVisible?520.f:154.f;
    float base=std::clamp(std::min(width/1280.f,height/720.f),.70f,1.5f);
    float scale=std::min({base,(width-24)/logicalWidth,(height-24)/logicalHeight});if(scale<=0)return;
    char key[320]{};std::snprintf(key,sizeof(key),"%d/%d/%d/%s/%.0f/%s/%.0f/%d/%d/%d/%d",width,height,controls.helpVisible,status.movement,status.speed,status.sky,status.sunAzimuth,controls.paused,status.shadows,status.color,status.fxaa);
    GLint oldProgram=0,oldVao=0,active=0,oldTexture=0,sampler=0;
    glGetIntegerv(GL_CURRENT_PROGRAM,&oldProgram);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&oldVao);glGetIntegerv(GL_ACTIVE_TEXTURE,&active);
    glActiveTexture(GL_TEXTURE0);glGetIntegerv(GL_TEXTURE_BINDING_2D,&oldTexture);glGetIntegeri_v(GL_SAMPLER_BINDING,0,&sampler);
    // Restore texture state even if native text rasterization reports a failure.
    try{if(cachedKey!=key){rasterize(controls.helpVisible,scale,controls,status);cachedKey=key;}}
    catch(...){glBindTexture(GL_TEXTURE_2D,oldTexture);glActiveTexture(active);throw;}
    GLboolean depth=glIsEnabled(GL_DEPTH_TEST),blend=glIsEnabled(GL_BLEND),cull=glIsEnabled(GL_CULL_FACE),scissor=glIsEnabled(GL_SCISSOR_TEST),offset=glIsEnabled(GL_POLYGON_OFFSET_FILL),srgb=glIsEnabled(GL_FRAMEBUFFER_SRGB),write=GL_TRUE,mask[4]{};
    GLint srcRGB,dstRGB,srcAlpha,dstAlpha,eqRGB,eqAlpha;
    glGetBooleanv(GL_DEPTH_WRITEMASK,&write);glGetBooleanv(GL_COLOR_WRITEMASK,mask);
    glGetIntegerv(GL_BLEND_SRC_RGB,&srcRGB);glGetIntegerv(GL_BLEND_DST_RGB,&dstRGB);glGetIntegerv(GL_BLEND_SRC_ALPHA,&srcAlpha);glGetIntegerv(GL_BLEND_DST_ALPHA,&dstAlpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB,&eqRGB);glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&eqAlpha);
    glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glDisable(GL_CULL_FACE);glDisable(GL_SCISSOR_TEST);glDisable(GL_POLYGON_OFFSET_FILL);glDisable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_BLEND);glBlendEquation(GL_FUNC_ADD);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glUseProgram(program);glBindVertexArray(vao);glBindTexture(GL_TEXTURE_2D,texture);glBindSampler(0,0);
    glUniform2f(viewportLocation,float(width),float(height));
    glUniform4f(rectangleLocation,controls.helpVisible?(width-textureWidth)*.5f:12.f,controls.helpVisible?(height-textureHeight)*.5f:12.f,float(textureWidth),float(textureHeight));
    glDrawArrays(GL_TRIANGLES,0,6);draws=1;
    glBindSampler(0,sampler);glBindTexture(GL_TEXTURE_2D,oldTexture);glActiveTexture(active);glBindVertexArray(oldVao);glUseProgram(oldProgram);
    glBlendFuncSeparate(srcRGB,dstRGB,srcAlpha,dstAlpha);glBlendEquationSeparate(eqRGB,eqAlpha);glDepthMask(write);glColorMask(mask[0],mask[1],mask[2],mask[3]);
    enable(GL_DEPTH_TEST,depth);enable(GL_BLEND,blend);enable(GL_CULL_FACE,cull);enable(GL_SCISSOR_TEST,scissor);enable(GL_POLYGON_OFFSET_FILL,offset);enable(GL_FRAMEBUFFER_SRGB,srgb);
}
