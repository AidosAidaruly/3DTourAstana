#pragma once

#include "Math3D.h"

#include <glad/glad.h>

#include <cstddef>

class SunShadows;

class KhanShatyr
{
public:
    explicit KhanShatyr(Vec3 position);
    ~KhanShatyr();

    KhanShatyr(const KhanShatyr&) = delete;
    KhanShatyr& operator=(const KhanShatyr&) = delete;

    void render(
        GLint modelLocation,
        GLint objectColorLocation,
        GLint khanCanopyLocation,
        GLint khanCableLocation,
        GLint shaftGlassLocation,
        GLint useGrassLocation,
        GLint useTextureLocation
    ) const;

    void renderShadow(SunShadows& shadows)const;

private:
    struct IndexRange
    {
        std::size_t firstIndex = 0;
        GLsizei count = 0;
    };

    void drawRange(const IndexRange& range, Vec3 color, GLint objectColorLocation) const;

    Mat4 model;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    IndexRange floorRange;
    IndexRange plinthRange;
    IndexRange wallRange;
    IndexRange upperWallRange;
    IndexRange panelSeamRange;
    IndexRange roofTransitionRange;
    IndexRange ventHousingRange;
    IndexRange ventCapRange;
    IndexRange canopyRange;
    IndexRange cableRange;
    IndexRange rimRange;
    IndexRange serviceRingRange;
    IndexRange serviceTrimRange;
    IndexRange mastRange;
    IndexRange approachRange;
    IndexRange stairRange;
    IndexRange lawnRange;
    IndexRange storefrontGlassRange;
    IndexRange doorGlassRange;
    IndexRange frameRange;
    IndexRange handleRange;
    IndexRange fasciaRange;
};
