#include "SunShadows.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace
{
    GLuint compile(GLenum type,const char* source)
    {
        GLuint shader=glCreateShader(type);glShaderSource(shader,1,&source,nullptr);glCompileShader(shader);
        GLint ok=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
        if(!ok){char log[4096]{};glGetShaderInfoLog(shader,4096,nullptr,log);glDeleteShader(shader);throw std::runtime_error(std::string("Sun depth shader: ")+log);}return shader;
    }
    void enable(GLenum cap,GLboolean value){if(value)glEnable(cap);else glDisable(cap);}
}
SunShadows::SunShadows()
{
    const char* vertex=R"GLSL(#version 330 core
layout(location=0) in vec3 position;
layout(location=3) in float material;
layout(location=4) in vec4 placement;
layout(location=5) in vec4 tintScale;
layout(location=6) in vec2 scaleXZ;
uniform mat4 lightMatrix,model;
uniform bool instanced;
flat out float kind;
void main(){
 vec3 world;
 if(instanced){float c=cos(placement.w),s=sin(placement.w);
  vec3 p=position*vec3(scaleXZ.x,tintScale.w,scaleXZ.y);
  world=vec3(c*p.x+s*p.z,p.y,-s*p.x+c*p.z)+placement.xyz;kind=material;
 }else{world=(model*vec4(position,1.0)).xyz;kind=0.0;}
 gl_Position=lightMatrix*vec4(world,1.0);
})GLSL";
    const char* fragment=R"GLSL(#version 330 core
flat in float kind;
void main(){
 // All residential glazing, including opaque reflective glass, is excluded.
 if((kind>0.5&&kind<1.5)||(kind>3.5&&kind<4.5))discard;
})GLSL";
    GLuint vs=compile(GL_VERTEX_SHADER,vertex),fs=0;
    try{fs=compile(GL_FRAGMENT_SHADER,fragment);}catch(...){glDeleteShader(vs);throw;}
    depthProgram=glCreateProgram();glAttachShader(depthProgram,vs);glAttachShader(depthProgram,fs);glLinkProgram(depthProgram);glDeleteShader(vs);glDeleteShader(fs);
    GLint ok=0;glGetProgramiv(depthProgram,GL_LINK_STATUS,&ok);
    if(!ok){char log[4096]{};glGetProgramInfoLog(depthProgram,4096,nullptr,log);glDeleteProgram(depthProgram);throw std::runtime_error(std::string("Sun depth link: ")+log);}
    matrixUniform=glGetUniformLocation(depthProgram,"lightMatrix");modelUniform=glGetUniformLocation(depthProgram,"model");instancedUniform=glGetUniformLocation(depthProgram,"instanced");
    GLint draw=0,read=0,activeUnit=0,texture=0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&draw);glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&read);glGetIntegerv(GL_ACTIVE_TEXTURE,&activeUnit);
    glActiveTexture(GL_TEXTURE2);glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture);
    glGenTextures(1,&depthTexture);glBindTexture(GL_TEXTURE_2D,depthTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,Settings::resolution,Settings::resolution,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_INT,nullptr);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
    const GLfloat border[]={1,1,1,1};glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,border);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_MODE,GL_COMPARE_REF_TO_TEXTURE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_FUNC,GL_LEQUAL);
    glGenFramebuffers(1,&framebuffer);glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,depthTexture,0);glDrawBuffer(GL_NONE);glReadBuffer(GL_NONE);
    GLenum status=glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,draw);glBindFramebuffer(GL_READ_FRAMEBUFFER,read);glBindTexture(GL_TEXTURE_2D,texture);glActiveTexture(activeUnit);
    if(status!=GL_FRAMEBUFFER_COMPLETE){glDeleteFramebuffers(1,&framebuffer);glDeleteTextures(1,&depthTexture);glDeleteProgram(depthProgram);throw std::runtime_error("Sun shadow framebuffer incomplete: "+std::to_string(status));}
}
SunShadows::~SunShadows(){glDeleteFramebuffers(1,&framebuffer);glDeleteTextures(1,&depthTexture);glDeleteProgram(depthProgram);}
bool SunShadows::prepare(const Mat4& view,Vec3 lightDirection)
{
    stats.updated=false;if(!active)return false;
    Vec3 eye{-(view[0]*view[12]+view[1]*view[13]+view[2]*view[14]),-(view[4]*view[12]+view[5]*view[13]+view[6]*view[14]),-(view[8]*view[12]+view[9]*view[13]+view[10]*view[14])};
    Vec3 forward{-view[2],-view[6],-view[10]};
    // Blend continuously into the ground intersection. G6A's hard pitch test
    // could jump the focus by tens of metres when looking down during flight.
    float depression=std::max(-forward.y,0.0f);
    float blend=std::clamp((depression-.08f)/.32f,0.0f,1.0f);blend=blend*blend*(3-2*blend);
    float groundAhead=std::clamp(std::max(eye.y,0.0f)/std::max(depression,.05f),0.0f,Settings::maximumAhead);
    float ahead=Settings::lookAhead+(groundAhead-Settings::lookAhead)*blend;
    Vec3 focus=eye+forward*ahead;focus.y=0;
    Vec3 direction=normalize(lightDirection),right=normalize(cross(direction,{0,1,0})),up=cross(right,direction);
    float texel=2*Settings::halfExtent/Settings::resolution;
    // Eight-texel steps preserve the same world-anchored texel lattice while
    // avoiding rebuilds on every small walking movement. PCF stays world locked.
    float centerStep=texel*Settings::centerStepTexels;
    focus=right*(std::round(dot(right,focus)/centerStep)*centerStep)+up*(std::round(dot(up,focus)/centerStep)*centerStep)+
        direction*(std::round(dot(direction,focus)/Settings::depthStep)*Settings::depthStep);
    Vec3 lightEye=focus-direction*Settings::lightDistance;
    Mat4 lightView{right.x,up.x,-direction.x,0,right.y,up.y,-direction.y,0,right.z,up.z,-direction.z,0,
        -dot(right,lightEye),-dot(up,lightEye),dot(direction,lightEye),1};
    Mat4 ortho{};ortho[0]=ortho[5]=1/Settings::halfExtent;ortho[10]=-2/(Settings::farPlane-Settings::nearPlane);
    ortho[14]=-(Settings::farPlane+Settings::nearPlane)/(Settings::farPlane-Settings::nearPlane);ortho[15]=1;
    Mat4 candidate{};for(int c=0;c<4;++c)for(int r=0;r<4;++r)for(int k=0;k<4;++k)candidate[c*4+r]+=ortho[k*4+r]*lightView[c*4+k];
    if(valid&&candidate==lightMatrix)return false;lightMatrix=candidate;return true;
}
bool SunShadows::intersects(Vec3 center,Vec3 extent)const
{
    const auto& m=lightMatrix;
    for(int axis=0;axis<3;++axis)for(float sign:{-1.0f,1.0f}){
        Vec3 n{m[3]+sign*m[axis],m[7]+sign*m[4+axis],m[11]+sign*m[8+axis]};
        if(dot(n,center)+m[15]+sign*m[12+axis]+std::abs(n.x)*extent.x+std::abs(n.y)*extent.y+std::abs(n.z)*extent.z<0)return false;
    }return true;
}
SunShadows::Pass::Pass(SunShadows& shadows):owner(shadows)
{
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&drawFbo);glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&readFbo);glGetIntegerv(GL_VIEWPORT,viewport);
    glGetIntegerv(GL_CURRENT_PROGRAM,&program);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);glGetIntegerv(GL_DEPTH_FUNC,&depthFunction);glGetIntegerv(GL_CULL_FACE_MODE,&cullMode);
    depth=glIsEnabled(GL_DEPTH_TEST);blend=glIsEnabled(GL_BLEND);cull=glIsEnabled(GL_CULL_FACE);offset=glIsEnabled(GL_POLYGON_OFFSET_FILL);scissor=glIsEnabled(GL_SCISSOR_TEST);
    glGetBooleanv(GL_DEPTH_WRITEMASK,&write);glGetBooleanv(GL_COLOR_WRITEMASK,colorWrite);glGetFloatv(GL_POLYGON_OFFSET_FACTOR,&factor);glGetFloatv(GL_POLYGON_OFFSET_UNITS,&units);glGetDoublev(GL_DEPTH_CLEAR_VALUE,&clearDepth);
    glBindFramebuffer(GL_FRAMEBUFFER,owner.framebuffer);glViewport(0,0,Settings::resolution,Settings::resolution);
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LESS);glDepthMask(GL_TRUE);glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
    glDisable(GL_BLEND);glDisable(GL_SCISSOR_TEST);glEnable(GL_CULL_FACE);glCullFace(GL_BACK);glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(Settings::rasterFactor,Settings::rasterUnits);
    glClearDepth(1);glClear(GL_DEPTH_BUFFER_BIT);glUseProgram(owner.depthProgram);glUniformMatrix4fv(owner.matrixUniform,1,GL_FALSE,owner.lightMatrix.data());
    owner.stats.draws=owner.stats.casters=0;owner.stats.triangles=0;
}
SunShadows::Pass::~Pass()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,drawFbo);glBindFramebuffer(GL_READ_FRAMEBUFFER,readFbo);glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
    glUseProgram(program);glBindVertexArray(vao);glDepthFunc(depthFunction);glDepthMask(write);glColorMask(colorWrite[0],colorWrite[1],colorWrite[2],colorWrite[3]);glCullFace(cullMode);glPolygonOffset(factor,units);glClearDepth(clearDepth);
    enable(GL_DEPTH_TEST,depth);enable(GL_BLEND,blend);enable(GL_CULL_FACE,cull);enable(GL_POLYGON_OFFSET_FILL,offset);enable(GL_SCISSOR_TEST,scissor);
    owner.valid=true;owner.stats.updated=true;++owner.stats.updates;
}
SunShadows::Binding::Binding(const SunShadows& shadows)
{
    glGetIntegerv(GL_ACTIVE_TEXTURE,&activeUnit);glActiveTexture(GL_TEXTURE2);glGetIntegerv(GL_TEXTURE_BINDING_2D,&previous);glBindTexture(GL_TEXTURE_2D,shadows.depthTexture);glActiveTexture(activeUnit);
}
SunShadows::Binding::~Binding(){glActiveTexture(GL_TEXTURE2);glBindTexture(GL_TEXTURE_2D,previous);glActiveTexture(activeUnit);}
SunShadows::Receiver SunShadows::receiver(GLuint program)
{
    Receiver r{glGetUniformLocation(program,"sunShadowEnabled"),glGetUniformLocation(program,"sunLightMatrix")};
    GLint previous=0;glGetIntegerv(GL_CURRENT_PROGRAM,&previous);glUseProgram(program);
    glUniform1i(glGetUniformLocation(program,"sunShadowMap"),2);
    glUniform4f(glGetUniformLocation(program,"sunShadowParams"),1.0f/Settings::resolution,
        Settings::minimumBias/(Settings::farPlane-Settings::nearPlane),Settings::slopeBias/(Settings::farPlane-Settings::nearPlane),Settings::edgeFade);
    glUniform1f(glGetUniformLocation(program,"sunShadowStrength"),Settings::shadowStrength);
    glUseProgram(previous);return r;
}
void SunShadows::apply(const Receiver& r)const
{
    glUniform1i(r.enabled,active&&valid);if(active&&valid)glUniformMatrix4fv(r.matrix,1,GL_FALSE,lightMatrix.data());
}
const char* SunShadows::shaderLibrary()
{
    return R"GLSL(
uniform bool sunShadowEnabled;
uniform mat4 sunLightMatrix;
uniform sampler2DShadow sunShadowMap;
uniform vec4 sunShadowParams;
uniform float sunShadowStrength;
float sunVisibility(vec3 world,vec3 normal,vec3 light)
{
 if(!sunShadowEnabled)return 1.0;
 vec3 p=(sunLightMatrix*vec4(world,1.0)).xyz*0.5+0.5;
 // Derivatives precede divergent coverage tests. Correct each PCF comparison
 // for the receiver plane rather than increasing bias for a wider filter.
 vec3 dx=dFdx(p),dy=dFdy(p);
 float ndl=clamp(dot(normal,light),0.0,1.0);
 if(ndl<=0.0||any(lessThanEqual(p,vec3(0.0)))||any(greaterThanEqual(p,vec3(1.0))))return 1.0;
 float determinant=dx.x*dy.y-dx.y*dy.x;
 vec2 gradient=vec2(0.0);
 if(abs(determinant)>1e-12)
  gradient=clamp(vec2(dy.y*dx.z-dx.y*dy.z,dx.x*dy.z-dy.x*dx.z)/determinant,vec2(-2.0),vec2(2.0));
 float slope=min(sqrt(max(1.0-ndl*ndl,0.0))/max(ndl,0.2),2.0);
 float bias=sunShadowParams.y+sunShadowParams.z*slope;
 // Exact separable 5x5 tent [1,2,3,2,1], bilinearly combined into 3x3
 // taps. Fraction-dependent weights stay continuous at texel boundaries.
 vec2 coordinate=p.xy/sunShadowParams.x-0.5;
 vec2 fraction=fract(coordinate),base=(floor(coordinate)+0.5)*sunShadowParams.x;
 vec3 wx=vec3(3.0-2.0*fraction.x,5.0,1.0+2.0*fraction.x);
 vec3 wy=vec3(3.0-2.0*fraction.y,5.0,1.0+2.0*fraction.y);
 vec3 ox=vec3(-2.0+(2.0-fraction.x)/wx.x,(2.0+fraction.x)/5.0,2.0+fraction.x/wx.z);
 vec3 oy=vec3(-2.0+(2.0-fraction.y)/wy.x,(2.0+fraction.y)/5.0,2.0+fraction.y/wy.z);
 float sum=0.0;
 for(int y=0;y<3;++y)for(int x=0;x<3;++x){
  vec2 sampleUV=base+vec2(ox[x],oy[y])*sunShadowParams.x;
  float receiverDepth=p.z+dot(gradient,sampleUV-p.xy);
  sum+=wx[x]*wy[y]*texture(sunShadowMap,vec3(sampleUV,receiverDepth-bias));
 }
 // Rounded coverage, not a visible rectangular curtain. Preserve the 30 m
 // axial fade and useful central coverage; only the outer corners taper sooner.
 vec2 q=abs(p.xy*2.0-1.0),q2=q*q;
 float radius=sqrt(sqrt(dot(q2,q2)));
 float fade=1.0-smoothstep(1.0-2.0*sunShadowParams.w,1.0,radius);
 return mix(1.0,sum/81.0,fade*sunShadowStrength);
}
)GLSL";
}
