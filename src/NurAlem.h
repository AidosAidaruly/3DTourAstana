#pragma once

#include "Math3D.h"
#include <glad/glad.h>
#include <cstddef>

// Closed glass sphere, tapered curtain-wall podium and batched exterior diagrid.
class NurAlem
{
public:
    static constexpr float radius = 40.0f;
    static constexpr float sphereCenterY = 60.3f;
    static constexpr float podiumRadius = 35.0f;
    static constexpr float seatHeight = 24.3f;
    explicit NurAlem(Vec3 site);
    ~NurAlem();
    NurAlem(const NurAlem&) = delete;
    NurAlem& operator=(const NurAlem&) = delete;
    void render(GLint modelLocation, GLint colorLocation, GLint glassLocation, GLint facadeLocation) const;

private:
    struct Range { std::size_t first = 0; GLsizei count = 0; };
    void draw(const Range& range) const;
    Mat4 model;
    GLuint vao = 0, vbo = 0, ebo = 0;
    Range stone, baseGlass, entranceGlass, entranceMetal, podiumMetal, sphere;
    Range diagonalSteel, latticeTies;
    mutable GLuint podiumProgram = 0;
    mutable GLint podiumGlassLocation = -1;
};
