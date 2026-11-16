#pragma once
#include "Math3D.h"
#include "BackgroundCityExpansionPlan.h"
#include <glad/glad.h>
#include <array>
#include <cstddef>

// Immutable archetypes and LOD buffers shared by all background-city phases.
class BackgroundResidential
{
public:
    struct Statistics {
        std::array<std::size_t,3> meshTriangles{},allBuildingTriangles{};
        std::size_t siteTriangles=0,gpuBytes=0;
        unsigned expansionTrees=0,expansionShrubs=0,expansionBenches=0;
        unsigned visibleBuildings=0,draws=0;std::size_t triangles=0;
        std::array<unsigned,3> visibleLods{};
    };
    BackgroundResidential();
    ~BackgroundResidential();
    BackgroundResidential(const BackgroundResidential&)=delete;
    BackgroundResidential& operator=(const BackgroundResidential&)=delete;
    void render(const Mat4& view,const Mat4& projection,Vec3 light,float ambient,float diffuse)const;
    const Statistics& statistics()const{return stats;}
private:
    struct Mesh {GLuint vbo=0,ebo=0;GLsizei count=0;};
    struct Instances {std::size_t first=0;GLsizei count=0;std::array<GLuint,3> vao{};};
    struct Group {
        Vec3 center{},extent{};std::array<Instances,BackgroundCityExpansionPlan::archetypeCount> types{};
        Mesh site;GLuint siteVao=0;mutable int lod=-1;mutable bool visible=false;
    };
    std::array<std::array<Mesh,3>,BackgroundCityExpansionPlan::archetypeCount> meshes{};
    std::array<Group,BackgroundCityExpansionPlan::groupCount> groups{};
    mutable Statistics stats;
    GLuint program=0,instanceBuffer=0;
    GLint viewLocation=-1,projectionLocation=-1,eyeLocation=-1,lightLocation=-1,ambientLocation=-1,diffuseLocation=-1;
    mutable bool viewValid=false;
    mutable Mat4 lastView{},lastProjection{};
};
