#pragma once

#include "Math3D.h"
#include <glad/glad.h>
#include <array>
#include <cstddef>

// Opaque, static, batched curtain-wall trio and common three-level stylobate.
class NorthernLights
{
public:
    struct Tower {float x,z,width,depth;int floors;};
    static constexpr float podiumHeight=11.8f,floorHeight=3.25f;
    // Residential floors above the three podium floors: total 39 / 44 / 34.
    static constexpr std::array<Tower,3> towers{{
        {-31,-1,23,26,36},{0,-9,24,27,41},{31,-1,22,25,31}}};
    NorthernLights(Vec3 position,float rotationDegrees);
    ~NorthernLights();
    NorthernLights(const NorthernLights&)=delete;
    NorthernLights& operator=(const NorthernLights&)=delete;
    void render(GLint modelLocation,GLint colorLocation,GLint objectLocation,
                GLint facadeLocation) const;
private:
    struct Range {std::size_t first=0;GLsizei count=0;};
    std::array<Range,13> ranges{};
    Mat4 model;
    GLuint vao=0,vbo=0,ebo=0;
};
