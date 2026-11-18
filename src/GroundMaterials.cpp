#include "GroundMaterials.h"
#include <array>
#include <cmath>
#include <cstdint>

namespace
{
    float hash(unsigned x,unsigned y)
    {
        std::uint32_t h=x*0x8da6b343u+y*0xd8163841u+0xcb1ab31fu;
        h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;h^=h>>16;
        return float(h&0xffffu)/65535.0f;
    }
    float noise(int x,int y,int cells)
    {
        const float u=float(x)*cells/256.0f,v=float(y)*cells/256.0f;
        const unsigned ix=static_cast<unsigned>(u),iy=static_cast<unsigned>(v),mask=cells-1;
        float a=u-ix,b=v-iy;a=a*a*(3-2*a);b=b*b*(3-2*b);
        const float lo=hash(ix&mask,iy&mask)*(1-a)+hash((ix+1)&mask,iy&mask)*a;
        const float hi=hash(ix&mask,(iy+1)&mask)*(1-a)+hash((ix+1)&mask,(iy+1)&mask)*a;
        return lo*(1-b)+hi*b;
    }
}
GroundMaterials::GroundMaterials()
{
    std::array<unsigned char,256*256*4> pixels{};
    for(int y=0;y<256;++y)for(int x=0;x<256;++x){const int i=(y*256+x)*4;
        pixels[i]=static_cast<unsigned char>(noise(x,y,8)*255);
        pixels[i+1]=static_cast<unsigned char>(noise(x,y,32)*255);
        pixels[i+2]=static_cast<unsigned char>(noise(x,y,64)*255);
        pixels[i+3]=static_cast<unsigned char>(hash(x,y)*255);}
    GLint active=0,previous=0;glGetIntegerv(GL_ACTIVE_TEXTURE,&active);
    glActiveTexture(GL_TEXTURE1);glGetIntegerv(GL_TEXTURE_BINDING_2D,&previous);
    glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,256,256,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,previous);glActiveTexture(active);
}
GroundMaterials::~GroundMaterials(){glDeleteTextures(1,&texture);}
GroundMaterials::Binding::Binding(const GroundMaterials& materials)
{
    glGetIntegerv(GL_ACTIVE_TEXTURE,&active);glActiveTexture(GL_TEXTURE1);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&previous);glBindTexture(GL_TEXTURE_2D,materials.texture);glActiveTexture(active);
}
GroundMaterials::Binding::~Binding()
{
    glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,previous);glActiveTexture(active);
}
void GroundMaterials::configure(GLuint program)
{
    GLint previous=0;glGetIntegerv(GL_CURRENT_PROGRAM,&previous);glUseProgram(program);
    glUniform1i(glGetUniformLocation(program,"groundDetail"),1);glUseProgram(previous);
}
const char* GroundMaterials::shaderLibrary()
{
    return R"GLSL(
uniform sampler2D groundDetail;
// Packed periodic value noise; all fine detail is hardware mip-filtered.
vec3 groundGrass(vec3 base,vec2 p)
{
    vec2 q=mat2(0.8,-0.6,0.6,0.8)*p;
    vec3 tone=texture(groundDetail,q/512.0).rgb-0.5;
    float grain=texture(groundDetail,p/4.0).a-0.5;
    return base*(1.0+dot(tone,vec3(0.12,0.065,0.025))+grain*0.045)
        +vec3(-0.008,0.006,-0.010)*tone.g;
}
vec3 groundAsphalt(vec3 base,vec2 p)
{
    vec2 tone=texture(groundDetail,p/128.0).rg-0.5;
    float grain=texture(groundDetail,p/8.0).a-0.5;
    return base*(1.0+dot(tone,vec2(0.055,0.035))+grain*0.10);
}
vec3 groundStoneFinish(vec3 base,vec2 p)
{
    float tone=texture(groundDetail,p/64.0).g-0.5;
    float grain=texture(groundDetail,p/4.0).a-0.5;
    return base*(1.0+tone*0.045+grain*0.035);
}
// Integral of a periodic narrow joint: stable even when many tiles cover a pixel.
float groundJointIntegral(float x,float width)
{
    return floor(x)*width+min(fract(x),width);
}
float groundJoint(float x,float footprint,float width)
{
    float span=max(footprint,0.0001),q=fract(x)+width*0.5;
    return clamp((groundJointIntegral(q+span*0.5,width)-
                  groundJointIntegral(q-span*0.5,width))/span,0.0,1.0);
}
vec3 groundPaving(vec3 base,vec2 p)
{
    vec2 grid=p/vec2(1.6,0.8),aa=fwidth(grid);
    grid.x+=0.5*mod(floor(grid.y),2.0);
    float seam=max(groundJoint(grid.x,aa.x,0.0044),groundJoint(grid.y,aa.y,0.0088));
    // Mip filtering also removes per-stone variation at a distance.
    float variation=texture(groundDetail,(floor(grid)+0.5)/256.0).a-0.5;
    float fade=1.0-smoothstep(0.25,1.0,max(aa.x,aa.y));
    return groundStoneFinish(base,p)*(1.0+variation*0.07*fade)*(1.0-seam*0.16);
}
)GLSL";
}
