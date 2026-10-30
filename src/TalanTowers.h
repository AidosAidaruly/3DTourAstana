#pragma once

#include "Math3D.h"
#include "Mesh.h"

#include <array>
#include <vector>

class TalanTowers
{
public:
    explicit TalanTowers(Vec3 position);
    ~TalanTowers();

    TalanTowers(const TalanTowers&) = delete;
    TalanTowers& operator=(const TalanTowers&) = delete;

    void render(
        const Mesh& mesh,
        GLint modelLocation,
        GLint objectColorLocation,
        GLint talanFacadeLocation
    ) const;

private:
    struct TowerRange
    {
        GLint firstVertex = 0;
        GLsizei sideVertexCount = 0;
        GLsizei roofVertexCount = 0;
        GLsizei edgeVertexCount = 0;
    };

    GLuint vao = 0;
    GLuint vbo = 0;
    std::array<TowerRange, 2> towerRanges;
    std::array<Mat4, 2> towerModels;
    GLint galleryGlassFirstVertex = 0;
    GLsizei galleryGlassVertexCount = 0;
    GLint canopyFirstVertex = 0;
    GLsizei canopyTopVertexCount = 0;
    GLsizei canopyUndersideVertexCount = 0;
    GLsizei canopyFasciaVertexCount = 0;
    Mat4 galleryModel;
    Mat4 podiumModel;
    Mat4 roofModel;
    std::vector<Mat4> storefrontFrames;
    std::vector<Mat4> entranceDoors;
    std::vector<Mat4> doorHandles;
    std::array<Mat4, 4> canopyColumns;
};
