#pragma once
#include "Math3D.h"
#include <glad/glad.h>
#include <cstddef>

// One static-scene local sun map. No cascades or scene/material ownership.
class SunShadows
{
public:
    struct Settings {
        static constexpr int resolution=2048;
        static constexpr float halfExtent=150,lightDistance=260,nearPlane=1,farPlane=600;
        static constexpr float minimumBias=.015f,slopeBias=.04f,edgeFade=.10f;
        static constexpr float shadowStrength=.94f,rasterFactor=.7f,rasterUnits=1;
        static constexpr float centerStepTexels=8,depthStep=16,lookAhead=45,maximumAhead=100;
        static constexpr int casterLod=2;
    };
    struct Receiver {GLint enabled=-1,matrix=-1;};
    struct Statistics {unsigned draws=0,casters=0,updates=0;std::size_t triangles=0;bool updated=false;};
    SunShadows();
    ~SunShadows();
    SunShadows(const SunShadows&)=delete;
    SunShadows& operator=(const SunShadows&)=delete;
    bool enabled()const{return active;}
    void setEnabled(bool value){active=value;}
    // Call after changing static caster transforms/content. Camera and sun
    // changes invalidate through prepare(); current city casters are static.
    void invalidate(){valid=false;}
    void updateToggle(bool down,bool focused){if(focused&&down&&!keyDown)active=!active;keyDown=down;}
    bool prepare(const Mat4& cameraView,Vec3 lightDirection);
    bool intersects(Vec3 center,Vec3 extent)const;
    void apply(const Receiver& receiver)const;
    static Receiver receiver(GLuint program);
    static const char* shaderLibrary();
    const Mat4& matrix()const{return lightMatrix;}
    const Statistics& statistics()const{return stats;}
    GLint modelLocation()const{return modelUniform;}
    void instanced(bool value)const{glUniform1i(instancedUniform,value);}
    void record(GLsizei indices,unsigned instances=1){++stats.draws;stats.triangles+=std::size_t(indices/3)*instances;}
    void countCasters(unsigned count){stats.casters+=count;}

    class Pass {
    public:
        explicit Pass(SunShadows& shadows);
        ~Pass();
        Pass(const Pass&)=delete;Pass& operator=(const Pass&)=delete;
    private:
        SunShadows& owner;
        GLint drawFbo=0,readFbo=0,viewport[4]{},program=0,vao=0,depthFunction=0,cullMode=0;
        GLboolean depth=false,write=false,blend=false,cull=false,offset=false,scissor=false,colorWrite[4]{};
        GLfloat factor=0,units=0;GLdouble clearDepth=1;
    };
    class Binding {
    public:
        explicit Binding(const SunShadows& shadows);
        ~Binding();
        Binding(const Binding&)=delete;Binding& operator=(const Binding&)=delete;
    private:GLint activeUnit=0,previous=0;
    };
private:
    bool active=true,keyDown=false,valid=false;
    GLuint framebuffer=0,depthTexture=0,depthProgram=0;
    GLint matrixUniform=-1,modelUniform=-1,instancedUniform=-1;
    Mat4 lightMatrix{};
    Statistics stats{};
};
