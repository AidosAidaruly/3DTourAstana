#pragma once
#include "Math3D.h"
#include <glad/glad.h>
#include <array>
#include <cstdint>
#include <vector>

// Stadium-local opaque renderer. Static terraces and three reusable chair LODs;
// no interaction with the city's transparent glass or vehicle renderers.
class AstanaArenaSeats
{
public:
    struct Statistics
    {
        unsigned total=0,lower=0,upper=0,white=0,staticTriangles=0;
        unsigned draws=0,triangles=0;
        std::array<unsigned,3> lods{},meshTriangles{};
        std::uint64_t uploads=0;
    };
    AstanaArenaSeats();
    ~AstanaArenaSeats();
    AstanaArenaSeats(const AstanaArenaSeats&)=delete;
    AstanaArenaSeats& operator=(const AstanaArenaSeats&)=delete;
    void render(const Mat4& view,const Mat4& projection,Vec3 light) const;
    Statistics statistics() const {return stats;}
private:
    struct Instance {Vec3 position;float white;float outwardX,outwardZ,shade,pad=0;};
    struct Block {unsigned first,count;Vec3 center;float radius;mutable unsigned lod=255;};
    struct Mesh
    {
        GLuint vao=0,vbo=0,ebo=0,instances=0;
        GLsizei indices=0;
        mutable unsigned count=0;
        mutable std::vector<unsigned> ids;
    };
    std::array<Mesh,3> seats{};
    Mesh concrete,rails;
    std::vector<Instance> placements;
    std::vector<Block> blocks;
    mutable std::array<std::vector<unsigned>,3> groups;
    mutable std::vector<Instance> scratch;
    mutable bool valid=false;
    mutable Vec3 lastEye{};
    mutable Statistics stats;
    GLuint program=0;
    GLint modelLocation=-1,viewLocation=-1,projectionLocation=-1,lightLocation=-1;
    GLint instancedLocation=-1,colorLocation=-1;
    void updateGroups(Vec3 localEye) const;
    void release();
};
