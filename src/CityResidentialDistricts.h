#pragma once
#include "Math3D.h"
#include "GrassDetail.h"
#include <glad/glad.h>
#include <array>
#include <cstddef>

// Five documented parcels only. A3 has its own unchanged premium renderer.
class CityResidentialDistricts
{
public:
    struct Building {int parcel,style,floors;float x,z,width,depth;int entrance;};
    // Styles: A=0 limestone mid-rise, B=1 brick/stone tower, C=2 framed mixed-height.
    static constexpr std::array<Building,13> buildings{{
        {0,0,12,-632,-665,24,46,1},{0,2,16,-523,-657,28,38,3},{0,0,10,-577,-692,42,22,2},
        {1,2,14,-439,-661,26,42,1},{1,0,12,-327,-656,26,38,3},{1,2,16,-383,-692,44,22,2},
        {3,1,22,-438,-795,28,36,1},{3,2,18,-328,-782,28,32,3},{3,1,26,-379,-837,42,26,2},
        {4,0,10,-317,82,22,48,1},{4,2,12,-264,96,28,28,3},
        {5,0,12,-330,190,26,30,3},{5,2,16,-315,244,24,30,3}}};
    struct Statistics {
        std::array<std::size_t,3> buildingTriangles{};
        std::array<unsigned,3> buildingDraws{};
        std::size_t siteTriangles=0;unsigned siteDraws=0;
        unsigned trees=0,shrubs=0,benches=0;
    };
    CityResidentialDistricts();
    ~CityResidentialDistricts();
    CityResidentialDistricts(const CityResidentialDistricts&)=delete;
    CityResidentialDistricts& operator=(const CityResidentialDistricts&)=delete;
    void render(const Mat4& view,const Mat4& projection,GLint model,GLint color,GLint glass,GLint brick,
                GLint grass,GLint texture,GLint repeat,GLuint paving) const;
    const Statistics& statistics()const{return stats;}
    void appendGrass(GrassDetail& detail)const{for(const auto& site:grassSites)detail.add(site);}
private:
    static constexpr int materials=12;
    struct Range {std::size_t first=0;GLsizei count=0;};
    struct Group {
        Vec3 center{},extent{};
        std::array<std::array<Range,materials>,3> levels{};
        std::array<Range,materials> site{};
        mutable int lod=-1;
    };
    std::array<Group,5> groups{};
    Statistics stats;
    std::array<GrassDetail::Site,5> grassSites;
    GLuint vao=0,vbo=0,ebo=0;
};
