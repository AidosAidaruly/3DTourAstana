#pragma once

#include <glad/glad.h>

// Owns reusable ground, unit-cube, and unit-sphere geometry in one VAO/VBO.
class Mesh
{
public:
    Mesh();
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void drawGround() const;
    void drawCube() const;
    void drawSphere() const;

private:
    GLuint vao = 0;
    GLuint vbo = 0;
    GLint sphereFirstVertex = 0;
    GLsizei sphereVertexCount = 0;
};
