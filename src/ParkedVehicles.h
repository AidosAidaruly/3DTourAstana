#pragma once
#include "Math3D.h"
#include <glad/glad.h>
#include <array>
#include <cstdint>

// Local opaque renderer; the city's mesh, shader and glass passes stay intact.
class ParkedVehicles
{
public:
    struct Statistics
    {
        std::array<unsigned,3> types{},lods{},visibleLods{};
        unsigned total=0,visible=0,draws=0,triangles=0,meshTriangles=0;
        std::uint64_t uploads=0,uploadedBytes=0;
    };
    ParkedVehicles();
    ~ParkedVehicles();
    ParkedVehicles(const ParkedVehicles&)=delete;
    ParkedVehicles& operator=(const ParkedVehicles&)=delete;
    void render(const Mat4& view,const Mat4& projection,Vec3 light,float ambient,float diffuse) const;
    Statistics statistics() const {return stats;}
private:
    struct Vehicle {Vec3 position,color;float direction=1;unsigned type=0,row=0,bank=0,bay=0;};
    struct Instance {Vec3 position;float direction;Vec3 color;};
    struct Mesh
    {
        GLuint vao=0,vbo=0,ebo=0,instances=0;
        GLsizei indices=0;
        mutable unsigned count=0;
        mutable std::array<unsigned,120> ids{};
    };
    std::array<Vehicle,120> vehicles{};
    std::array<Mesh,9> meshes{};
    mutable std::array<unsigned char,120> lod{};
    mutable Mat4 lastView{},lastProjection{};
    mutable bool groupingValid=false;
    mutable Statistics stats{};
    mutable std::array<Instance,120> uploadScratch{};
    GLuint program=0;
    GLint viewLocation=-1,projectionLocation=-1,eyeLocation=-1,lightLocation=-1,ambientLocation=-1,diffuseLocation=-1;
    void release();
    void updateGroups(const Mat4& view,const Mat4& projection,Vec3 eye) const;
};
