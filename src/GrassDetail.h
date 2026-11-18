#pragma once
#include "Math3D.h"
#include "SunShadows.h"
#include <glad/glad.h>
#include <vector>

// Opt-in lawn surfaces, captured from existing mesh/material ranges at startup.
// Unclassified land never receives grass. No city-wide terrain guessing.
class GrassDetail
{
public:
    struct Triangle {Vec3 a,b,c;};
    struct Disc {Vec3 center;float radius;};
    struct Site {
        std::vector<Triangle> lawn,blocked;
        std::vector<Disc> planting;
        Vec3 color{.37f,.53f,.35f};
    };
    struct Statistics {
        unsigned sites=0,cells=0,tufts=0,visibleCells=0,draws=0;
        std::size_t triangles=0,bytes=0;
    };
    void add(const Site& site);
    void upload();
    void render(const Mat4& view,const Mat4& projection,Vec3 sun,float ambient,
                float direct,const SunShadows& shadows)const;
    void setEnabled(bool value){enabled=value;}
    const Statistics& statistics()const{return stats;}
    GrassDetail()=default;
    ~GrassDetail();
    GrassDetail(const GrassDetail&)=delete;
    GrassDetail& operator=(const GrassDetail&)=delete;
private:
    struct Cell {Vec3 center,extent;GLuint vao=0;GLsizei count=0;};
    std::vector<Site> sites;
    std::vector<Cell> cells;
    GLuint program=0,vbo=0,ebo=0,instances=0;
    GLsizei indexCount=0;
    GLint viewLocation=-1,projectionLocation=-1,eyeLocation=-1,sunLocation=-1,
          ambientLocation=-1,directLocation=-1;
    SunShadows::Receiver receiver{};
    bool enabled=true;
    mutable Statistics stats;
};
