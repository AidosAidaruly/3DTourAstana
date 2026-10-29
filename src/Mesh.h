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
    void drawCircularPlaza() const;
    void drawCircularBorder() const;
    void drawGardenLawn() const;
    void drawGardenInnerBand() const;
    void drawGardenSoil() const;
    void drawGardenBedBorder() const;
    void drawFlowerBlossom() const;

private:
    GLuint vao = 0;
    GLuint vbo = 0;
    GLint sphereFirstVertex = 0;
    GLsizei sphereVertexCount = 0;
    GLint tipFirstVertex = 0;
    GLint circularPlazaFirstVertex = 0;
    GLint circularBorderFirstVertex = 0;
    GLint gardenLawnFirstVertex = 0;
    GLint gardenInnerBandFirstVertex = 0;
    GLint gardenSoilFirstVertex = 0;
    GLint gardenBorderFirstVertex = 0;
    GLsizei gardenBorderVertexCount = 0;
    GLint flowerBlossomFirstVertex = 0;
};
