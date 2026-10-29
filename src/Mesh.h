#pragma once

#include <glad/glad.h>

// Owns shared ground, cube, sphere, and tapered-tip geometry in one VAO/VBO.
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
    void drawPointedTip() const;

private:
    GLuint vao = 0;
    GLuint vbo = 0;
    GLint sphereFirstVertex = 0;
    GLsizei sphereVertexCount = 0;
    GLint tipFirstVertex = 0;
};
