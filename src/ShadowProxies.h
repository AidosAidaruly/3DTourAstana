#pragma once
#include "SunShadows.h"
#include <array>
#include <map>
#include <vector>

// Position-only, initialization-built silhouettes submitted through the EXISTING
// SunShadows depth program. No visible geometry, materials or renderer ownership.
class ShadowProxies
{
public:
    enum Kind {Box,Ellipsoid,Cone,Cylinder};
    struct Shape {Kind kind;Mat4 model;bool detailed=false;};
    using Shapes=std::vector<Shape>;
    struct Statistics {std::size_t triangles=0,bytes=0;unsigned groups=0,shapes=0;};
    ShadowProxies()=default;
    ~ShadowProxies();
    ShadowProxies(const ShadowProxies&)=delete;
    ShadowProxies& operator=(const ShadowProxies&)=delete;
    void beginLandmark(){landmarkGroup=nextLandmark++;}
    void beginVegetation(){landmarkGroup=-1;}
    void add(Kind kind,const Mat4& model,bool detailed=false);
    void append(const Shapes& shapes){for(const auto& s:shapes)add(s.kind,s.model,s.detailed);}
    static Mat4 compose(const Mat4& parent,const Mat4& local);
    static void tree(Shapes& shapes,const Mat4& parent,Vec3 root,float scale,int kind);
    void upload();
    void render(SunShadows& shadows)const;
    const Statistics& statistics()const{return stats;}
private:
    struct Batch {std::vector<Vec3> vertices;std::vector<GLuint> indices;unsigned shapes=0;};
    struct Range {std::size_t first=0;GLsizei count=0;Vec3 center{},extent{};unsigned shapes=0;};
    std::map<std::array<int,3>,Batch> building;
    std::vector<Range> ranges;
    int landmarkGroup=-1,nextLandmark=0;
    GLuint vao=0,vbo=0,ebo=0;
    Statistics stats{};
};
