#pragma once
#include "Math3D.h"
#include <glad/glad.h>
#include <array>
#include <cstddef>

// Three independent 22/28/34-storey towers in the northeast EXPO block.
// Local plot: X=-64..72, Z=-64..72; the approved streets remain untouched.
class ExpoPremiumResidence
{
public:
    static constexpr Vec3 anchor{-586,0,132};
    struct Statistics {
        unsigned windows=0,loggias=0,arches=0,entrances=0,doorLeaves=0;
        std::size_t triangles=0;
    };
    ExpoPremiumResidence();
    ~ExpoPremiumResidence();
    ExpoPremiumResidence(const ExpoPremiumResidence&)=delete;
    ExpoPremiumResidence& operator=(const ExpoPremiumResidence&)=delete;
    const Statistics& statistics() const { return stats; }
    void render(GLint modelLocation,GLint colorLocation,GLint glassLocation,
                GLint grassLocation,GLint textureLocation,GLint repeatLocation,GLuint pavingTexture) const;
private:
    struct Range {std::size_t first=0;GLsizei count=0;};
    Mat4 model=modelMatrix(anchor,0,{1,1,1});
    GLuint vao=0,vbo=0,ebo=0;
    mutable GLuint materialProgram=0;
    mutable GLint brickLocation=-1;
    std::array<Range,10> ranges;
    Statistics stats;
};
