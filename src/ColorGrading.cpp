#include "ColorGrading.h"
#include <stdexcept>
#include <string>

namespace
{
    const char* vertexSource=R"GLSL(#version 330 core
out vec2 uv;
void main(){
    vec2 p=vec2(gl_VertexID==1?3.0:-1.0,gl_VertexID==2?3.0:-1.0);
    uv=p*0.5+0.5;gl_Position=vec4(p,0.0,1.0);
})GLSL";
    const char* fragmentSource=R"GLSL(#version 330 core
in vec2 uv;
out vec4 fragColor;
uniform sampler2D sceneColor;
uniform vec4 grade; // exposure EV, contrast, saturation, highlight knee
uniform bvec2 effects; // color grading, FXAA (independent F7 / F8)
vec3 decodeDisplay(vec3 c){
    return mix(c/12.92,pow((c+0.055)/1.055,vec3(2.4)),step(vec3(0.04045),c));
}
vec3 encodeDisplay(vec3 c){
    return mix(c*12.92,1.055*pow(c,vec3(1.0/2.4))-0.055,step(vec3(0.0031308),c));
}
vec3 outputColor(vec3 source){
    if(!effects.x)return clamp(source,0.0,1.0);
    // The legacy renderer authors/blends display-referred colors without
    // gamma correction. Decode that COMPOSED image, never individual panes.
    // Floating storage retains highlights above 1 until this global shoulder.
    vec3 linear=decodeDisplay(max(source,vec3(0.0)));
    float y=dot(linear,vec3(0.2126,0.7152,0.0722));
    float exposed=y*exp2(grade.x);
    // Pivoted contrast preserves black and never clips dark facade detail.
    float mapped=0.18*pow(exposed/0.18,grade.y);
    float high=max(mapped-grade.w,0.0),range=1.0-grade.w;
    mapped=min(mapped,grade.w)+high/(1.0+high/range);
    linear*=mapped/max(y,0.000001);
    linear=mix(vec3(mapped),linear,grade.z);
    // Compress only out-of-gamut highlights toward equal-luminance white,
    // avoiding hard per-channel clipping on gold and saturated reflections.
    float peak=max(max(linear.r,linear.g),linear.b);
    if(peak>1.0)linear=mix(vec3(mapped),linear,clamp((1.0-mapped)/max(peak-mapped,0.000001),0.0,1.0));
    return encodeDisplay(max(linear,vec3(0.0)));
}
float luma(vec3 color){return dot(color,vec3(0.299,0.587,0.114));}
vec3 rawPixel(ivec2 p,ivec2 size){return texelFetch(sceneColor,clamp(p,ivec2(0),size-1),0).rgb;}
vec3 outputSample(vec2 p){return outputColor(texture(sceneColor,p).rgb);}
vec3 antialias(vec3 center,ivec2 pixel,ivec2 size){
    vec3 nw=rawPixel(pixel+ivec2(-1, 1),size),ne=rawPixel(pixel+ivec2(1, 1),size);
    vec3 sw=rawPixel(pixel+ivec2(-1,-1),size),se=rawPixel(pixel+ivec2(1,-1),size);
    float m=luma(clamp(center,0.0,1.0)),a=luma(clamp(nw,0.0,1.0)),b=luma(clamp(ne,0.0,1.0)),
          c=luma(clamp(sw,0.0,1.0)),d=luma(clamp(se,0.0,1.0));
    float low=min(m,min(min(a,b),min(c,d))),high=max(m,max(max(a,b),max(c,d)));
    // Fast rejection in the legacy display-referred source. Do not run four
    // extra tone curves across smooth sky/grass or faint distant facade detail.
    if(high-low<max(0.045,high*0.166))return outputColor(center);
    vec2 direction=vec2(-((a+b)-(c+d)),(b+d)-(a+c));
    float reduce=max((a+b+c+d)*(0.25*0.125),1.0/128.0);
    direction=clamp(direction/(min(abs(direction.x),abs(direction.y))+reduce),vec2(-6.0),vec2(6.0))/vec2(size);
    vec2 p=(vec2(pixel)+0.5)/vec2(size);
    // Filter the final display colors, AFTER each selected sample's unchanged
    // G7C transform. Edge detection is cheap; never blend linear-light colors
    // and gamma-encode the result a second time.
    vec3 inner=0.5*(outputSample(p-direction/6.0)+outputSample(p+direction/6.0));
    vec3 outer=inner*0.5+0.25*(outputSample(p-direction*0.5)+outputSample(p+direction*0.5));
    // Compare in output space too. Only the two extrema need grading, rather
    // than grading all four diagonal pixels before the early-out.
    vec3 darkest=center,brightest=center;
    if(a<m&&a<=min(b,min(c,d)))darkest=nw;
    else if(b<m&&b<=min(c,d))darkest=ne;
    else if(c<m&&c<=d)darkest=sw;
    else if(d<m)darkest=se;
    if(a>m&&a>=max(b,max(c,d)))brightest=nw;
    else if(b>m&&b>=max(c,d))brightest=ne;
    else if(c>m&&c>=d)brightest=sw;
    else if(d>m)brightest=se;
    float candidate=luma(outer),minimum=luma(outputColor(darkest)),maximum=luma(outputColor(brightest));
    vec3 filtered=candidate<minimum||candidate>maximum?inner:outer;
    // Retain a little center coverage so narrow frames/markings stay crisp.
    return mix(outputColor(center),filtered,0.85);
}
void main(){
    ivec2 size=textureSize(sceneColor,0),pixel=clamp(ivec2(uv*vec2(size)),ivec2(0),size-1);
    vec3 center=rawPixel(pixel,size);
    vec3 display=effects.y?antialias(center,pixel,size):outputColor(center);
    float dither=(fract(dot(gl_FragCoord.xy,vec2(0.75487766,0.56984029)))-0.5)/255.0;
    fragColor=vec4(display+vec3(effects.x?dither:0.0),1.0);
})GLSL";
    GLuint compile(GLenum type,const char* source)
    {
        GLuint shader=glCreateShader(type);glShaderSource(shader,1,&source,nullptr);glCompileShader(shader);
        GLint ok=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
        if(!ok){char log[4096]{};glGetShaderInfoLog(shader,4096,nullptr,log);glDeleteShader(shader);
            throw std::runtime_error(std::string("Color grading shader: ")+log);}return shader;
    }
    void enable(GLenum flag,GLboolean value){if(value)glEnable(flag);else glDisable(flag);}
}

ColorGrading::ColorGrading()
{
    GLuint vertex=compile(GL_VERTEX_SHADER,vertexSource),fragment=0;
    try{fragment=compile(GL_FRAGMENT_SHADER,fragmentSource);}catch(...){glDeleteShader(vertex);throw;}
    program=glCreateProgram();glAttachShader(program,vertex);glAttachShader(program,fragment);glLinkProgram(program);
    glDeleteShader(vertex);glDeleteShader(fragment);GLint ok=0;glGetProgramiv(program,GL_LINK_STATUS,&ok);
    if(!ok){char log[4096]{};glGetProgramInfoLog(program,4096,nullptr,log);glDeleteProgram(program);
        throw std::runtime_error(std::string("Color grading link: ")+log);}
    GLint previous=0;glGetIntegerv(GL_CURRENT_PROGRAM,&previous);glUseProgram(program);
    glUniform1i(glGetUniformLocation(program,"sceneColor"),0);
    effectsLocation=glGetUniformLocation(program,"effects");
    glUniform4f(glGetUniformLocation(program,"grade"),Settings::exposureEV,Settings::contrast,Settings::saturation,Settings::highlightKnee);
    glUseProgram(previous);glGenVertexArrays(1,&vao);
    glGenTextures(1,&color);glGenRenderbuffers(1,&depth);glGenFramebuffers(1,&framebuffer);
}
ColorGrading::~ColorGrading()
{
    glDeleteFramebuffers(1,&framebuffer);glDeleteRenderbuffers(1,&depth);glDeleteTextures(1,&color);
    glDeleteVertexArrays(1,&vao);glDeleteProgram(program);
}
void ColorGrading::updateToggle(bool down,bool focused)
{
    if(focused&&down&&!wasPressed)on=!on;
    wasPressed=down;
}
void ColorGrading::updateFxaaToggle(bool down,bool focused)
{
    if(focused&&down&&!aaWasPressed)aaOn=!aaOn;
    aaWasPressed=down;
}
void ColorGrading::resize(int w,int h)
{
    if(w==width&&h==height)return;
    GLint draw=0,read=0,texture=0,renderbuffer=0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&draw);glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&read);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture);glGetIntegerv(GL_RENDERBUFFER_BINDING,&renderbuffer);
    glBindTexture(GL_TEXTURE_2D,color);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16F,w,h,0,GL_RGBA,GL_HALF_FLOAT,nullptr);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glBindRenderbuffer(GL_RENDERBUFFER,depth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,w,h);
    glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);glReadBuffer(GL_COLOR_ATTACHMENT0);
    GLenum status=glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,draw);glBindFramebuffer(GL_READ_FRAMEBUFFER,read);
    glBindTexture(GL_TEXTURE_2D,texture);glBindRenderbuffer(GL_RENDERBUFFER,renderbuffer);
    if(status!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("Incomplete color grading framebuffer: "+std::to_string(status));
    width=w;height=h;
}
ColorGrading::Frame::Frame(ColorGrading& owner,int width,int height):owner(owner)
{
    if((!owner.on&&!owner.aaOn)||width<=0||height<=0)return;
    owner.resize(width,height);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&draw);glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&read);
    glGetIntegerv(GL_VIEWPORT,viewport);srgb=glIsEnabled(GL_FRAMEBUFFER_SRGB);active=true;
    glDisable(GL_FRAMEBUFFER_SRGB);glBindFramebuffer(GL_FRAMEBUFFER,owner.framebuffer);glViewport(0,0,width,height);
}
void ColorGrading::Frame::restoreTarget()const
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,draw);glBindFramebuffer(GL_READ_FRAMEBUFFER,read);
    glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);enable(GL_FRAMEBUFFER_SRGB,srgb);
}
ColorGrading::Frame::~Frame(){if(active)restoreTarget();}
void ColorGrading::Frame::present()
{
    if(!active||presented)return;
    GLint previousProgram=0,previousVao=0,unit=0,texture=0,sampler=0;
    glGetIntegerv(GL_CURRENT_PROGRAM,&previousProgram);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&previousVao);glGetIntegerv(GL_ACTIVE_TEXTURE,&unit);
    glActiveTexture(GL_TEXTURE0);glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture);glGetIntegeri_v(GL_SAMPLER_BINDING,0,&sampler);
    GLboolean depthTest=glIsEnabled(GL_DEPTH_TEST),blend=glIsEnabled(GL_BLEND),cull=glIsEnabled(GL_CULL_FACE),
        scissor=glIsEnabled(GL_SCISSOR_TEST),offset=glIsEnabled(GL_POLYGON_OFFSET_FILL),write=GL_TRUE,mask[4]{};
    glGetBooleanv(GL_DEPTH_WRITEMASK,&write);glGetBooleanv(GL_COLOR_WRITEMASK,mask);
    restoreTarget();glDisable(GL_FRAMEBUFFER_SRGB);glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glDisable(GL_SCISSOR_TEST);glDisable(GL_POLYGON_OFFSET_FILL);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glUseProgram(owner.program);glBindVertexArray(owner.vao);glBindTexture(GL_TEXTURE_2D,owner.color);glBindSampler(0,0);
    glUniform2i(owner.effectsLocation,owner.on?1:0,owner.aaOn?1:0);
    glDrawArrays(GL_TRIANGLES,0,3);
    glBindTexture(GL_TEXTURE_2D,texture);glBindSampler(0,sampler);glActiveTexture(unit);glUseProgram(previousProgram);glBindVertexArray(previousVao);
    glDepthMask(write);glColorMask(mask[0],mask[1],mask[2],mask[3]);enable(GL_DEPTH_TEST,depthTest);enable(GL_BLEND,blend);enable(GL_CULL_FACE,cull);
    enable(GL_SCISSOR_TEST,scissor);enable(GL_POLYGON_OFFSET_FILL,offset);enable(GL_FRAMEBUFFER_SRGB,srgb);presented=true;
}
