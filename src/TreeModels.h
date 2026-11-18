#pragma once
#include "Math3D.h"
#include "SunShadows.h"
#include <glad/glad.h>
#include <array>
#include <vector>

class ShadowProxies;
// Shared, opaque tree geometry. Plantings retain their authored roots and
// conservative envelopes; all mesh/instance buffers are initialization-only.
class TreeModels
{
public:
    struct Planting {Vec3 root;float radius,height,yaw,tint;int variant;bool proxy;};
    struct Statistics {
        unsigned trees=0,cells=0,visible=0,draws=0;
        std::array<unsigned,5> variants{};
        std::array<unsigned,3> visibleLods{};
        std::array<std::size_t,3> meshTriangles{};
        std::size_t triangles=0,bytes=0;
    };
    // category 0 broadleaf, 1 columnar broadleaf, 2 conifer.
    static Planting planting(Vec3 root,float radius,float height,int category=0,bool proxy=false);
    void add(Planting p){plantings.push_back(p);}
    void append(const std::vector<Planting>& p){plantings.insert(plantings.end(),p.begin(),p.end());}
    void upload();
    void render(const Mat4& view,const Mat4& projection,Vec3 sun,float ambient,float direct,const SunShadows& shadows)const;
    void appendShadowProxies(ShadowProxies& shadows)const;
    const Statistics& statistics()const{return stats;}
    TreeModels()=default;
    ~TreeModels();
    TreeModels(const TreeModels&)=delete;
    TreeModels& operator=(const TreeModels&)=delete;
private:
    struct Mesh {GLuint vbo=0,ebo=0;GLsizei count=0;};
    struct Range {unsigned first=0;GLsizei count=0;std::array<GLuint,3> vao{};};
    struct Cell {Vec3 center{},extent{};std::array<Range,5> types{};mutable int lod=-1;mutable bool visible=false;};
    std::vector<Planting> plantings;
    std::vector<Cell> cells;
    std::array<std::array<Mesh,3>,5> meshes{};
    GLuint program=0,instances=0;
    GLint viewLocation=-1,projectionLocation=-1,eyeLocation=-1,sunLocation=-1,ambientLocation=-1,directLocation=-1;
    SunShadows::Receiver receiver{};
    mutable Statistics stats;
    mutable bool cached=false;
    mutable Mat4 lastView{},lastProjection{};
};
