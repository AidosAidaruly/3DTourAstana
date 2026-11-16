#pragma once
#include "Math3D.h"
#include <glad/glad.h>
#include <array>
#include <cstddef>

// One completed residential block, confined to documented parcel A3.
class CityPremiumResidence
{
public:
    static constexpr Vec3 anchor{-577.5f,0,-803};
    struct Building {
        float x,z,width,depth;int floors,xBays,zBays;
        float setback,finalSetback,crown;int mainSide,streetSide,style;
    };
    static constexpr std::array<Building,3> buildings{{
        {-52,5,28,30,16,7,7,1.4f,.6f,1.55f,1,3,0},
        {52,7,30,32,22,7,7,1.6f,.8f,1.85f,3,1,1},
        {0,-34,40,28,28,9,7,1.8f,1.0f,2.2f,2,-1,2}}};
    struct Statistics {unsigned windows=0,loggias=0,arches=0,entrances=0;std::size_t baseTriangles=0,frameTriangles=0,fineTriangles=0;};
    CityPremiumResidence();
    ~CityPremiumResidence();
    CityPremiumResidence(const CityPremiumResidence&)=delete;
    CityPremiumResidence& operator=(const CityPremiumResidence&)=delete;
    void render(const Mat4& view,const Mat4& projection,GLint modelLocation,GLint colorLocation,GLint glassLocation,
                GLint brickLocation,GLint grassLocation,GLint textureLocation,GLint repeatLocation,GLuint pavingTexture) const;
    const Statistics& statistics() const{return stats;}
    int detailLevel() const{return lod;}
private:
    struct Range {std::size_t first=0;GLsizei count=0;};
    std::array<Range,14> ranges{};
    Mat4 model=modelMatrix(anchor,0,{1,1,1});
    GLuint vao=0,vbo=0,ebo=0;
    Statistics stats;
    mutable int lod=0;
    mutable bool lodInitialized=false;
};
