#pragma once
#include "Math3D.h"
#include <glad/glad.h>
#include <string>

namespace DayAtmosphere
{
    // Ray travel direction, shared by scene lighting, sky glow and shadows.
    // ~50 degrees elevation, same G7A azimuth: readable vertical facades
    // without sunset-length shadows. Never maintain a separate sky sun.
    inline constexpr Vec3 sunRayDirection{-0.72f,-1.0f,-0.432f};
    // Pure GLSL: also used by vertex-lit instanced window LODs.
    const char* daylightSource();
    // Initialization-only GLSL composition. All geometry shaders share exactly
    // the same aerial perspective; material alpha and early returns survive.
    std::string fragmentSource(std::string source,bool applyHaze=true);
}

class SkyAtmosphere
{
public:
    enum class Mode {Realistic,Anime};
    SkyAtmosphere();
    ~SkyAtmosphere();
    SkyAtmosphere(const SkyAtmosphere&)=delete;
    SkyAtmosphere& operator=(const SkyAtmosphere&)=delete;
    void render(const Mat4& view,const Mat4& projection)const;
    void updateToggle(bool down,bool focused){if(focused&&down&&!keyDown)mode=mode==Mode::Anime?Mode::Realistic:Mode::Anime;keyDown=down;}
    void setMode(Mode value){mode=value;}
    Mode currentMode()const{return mode;}
    const char* modeName()const{return mode==Mode::Anime?"ANIME":"CINEMATIC";}
    bool setSunOffset(float degrees);
    float sunOffsetDegrees()const{return sunOffset;}
    Vec3 lightRayDirection()const{return rays;}
    class SunBinding
    {
    public:
        explicit SunBinding(const SkyAtmosphere& sky);
        ~SunBinding();
        SunBinding(const SunBinding&)=delete;
        SunBinding& operator=(const SunBinding&)=delete;
    private:
        GLint previous=0,generic=0;
        bool changed=false;
    };
private:
    GLuint program=0,vao=0,sunBuffer=0;
    std::array<GLuint,2> cloudTextures{};
    GLint viewLocation=-1,projectionLocation=-1,animeLocation=-1;
    Mode mode=Mode::Realistic;
    bool keyDown=false;
    float sunOffset=0;
    Vec3 rays=DayAtmosphere::sunRayDirection;
};
