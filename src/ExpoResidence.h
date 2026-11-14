#pragma once
#include "Math3D.h"
#include <glad/glad.h>
#include <array>
#include <cstddef>

// One U-shaped residential development in the northwest EXPO urban block.
class ExpoResidence
{
public:
    static constexpr Vec3 anchor{-991,0,65};
    static constexpr float bodyWidth=72,bodyDepth=70;
    struct Statistics { unsigned windows=0,balconies=0,loggias=0,entrances=0,doorLeaves=0; };
    ExpoResidence();
    ~ExpoResidence();
    ExpoResidence(const ExpoResidence&)=delete;
    ExpoResidence& operator=(const ExpoResidence&)=delete;
    const Statistics& statistics() const { return stats; }
    void render(GLint modelLocation,GLint colorLocation,GLint glassLocation,
                GLint grassLocation,GLint textureLocation,GLint repeatLocation,GLuint pavingTexture) const;
private:
    struct Range {std::size_t first=0;GLsizei count=0;};
    void draw(const Range& range) const;
    Mat4 model=modelMatrix(anchor,0,{1,1,1});
    GLuint vao=0,vbo=0,ebo=0;
    std::array<Range,10> ranges;
    Statistics stats;
};
