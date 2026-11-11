#pragma once

#include "Math3D.h"
#include <glad/glad.h>
#include <cstddef>

// Indexed glass sphere, circular base and separately batched exterior diagrid.
class NurAlem
{
public:
    static constexpr float radius = 40.0f;
    static constexpr float sphereCenterY = 60.3f;
    static constexpr float podiumRadius = 34.0f;
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
    Range stone, baseGlass, entranceGlass, entranceMetal, sphere;
    Range diagonalSteel, latticeTies;
};
