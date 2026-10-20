#pragma once

#include <glad/glad.h>

// Owns the existing ground and unit-cube vertex data in one VAO/VBO.
class Mesh
{
public:
    Mesh();
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void drawGround() const;
    void drawCube() const;

private:
    GLuint vao = 0;
    GLuint vbo = 0;
};
