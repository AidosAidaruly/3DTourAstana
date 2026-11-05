#pragma once

#include <glad/glad.h>

#include <array>

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
    void drawUnitDisk() const;
    void drawUnitRing() const;
    void drawUnitCylinderWall() const;
    void drawFountainJets() const;
    void drawUnitCapsule() const;
    void drawCeremonialCourt() const;
    void drawParterreStems() const;
    void drawParterreFlowers(int colorIndex) const;
    void drawApproachFlowerStems() const;
    void drawApproachFlowers(int colorIndex) const;
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
    GLint unitDiskFirstVertex = 0;
    GLint unitRingFirstVertex = 0;
    GLint unitCylinderWallFirstVertex = 0;
    GLint fountainJetFirstVertex = 0;
    GLsizei fountainJetVertexCount = 0;
    GLint unitCapsuleFirstVertex = 0;
    GLsizei unitCapsuleVertexCount = 0;
    GLint ceremonialCourtFirstVertex = 0;
    GLsizei ceremonialCourtVertexCount = 0;
    GLint parterreStemFirstVertex = 0;
    GLsizei parterreStemVertexCount = 0;
    std::array<GLint, 4> parterreFlowerFirstVertex{};
    std::array<GLsizei, 4> parterreFlowerVertexCount{};
    GLint approachStemFirstVertex = 0;
    GLsizei approachStemVertexCount = 0;
    std::array<GLint, 2> approachFlowerFirstVertex{};
    std::array<GLsizei, 2> approachFlowerVertexCount{};
    GLint gardenLawnFirstVertex = 0;
    GLint gardenInnerBandFirstVertex = 0;
    GLint gardenSoilFirstVertex = 0;
    GLint gardenBorderFirstVertex = 0;
    GLsizei gardenBorderVertexCount = 0;
    GLint flowerBlossomFirstVertex = 0;
};
