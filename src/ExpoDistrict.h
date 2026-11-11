#pragma once

#include "Math3D.h"
#include "NurAlem.h"
#include "ExpoPavilion.h"
#include <glad/glad.h>
#include <cstddef>
#include <vector>

// Four-petal EXPO ensemble, central sphere, four low pods and pedestrian courts.
class ExpoDistrict
{
public:
    static constexpr Vec3 center{-765, 0, -130};
    static constexpr float halfWidth = 180, halfDepth = 200;
    static constexpr float nurAlemRadius = 40;
    static constexpr float centralPadRadius = 48, centralPlazaRadius = 62;
    static constexpr float pavilionInnerRadius = 92, pavilionOuterRadius = 132;

    ExpoDistrict();
    ~ExpoDistrict();
    ExpoDistrict(const ExpoDistrict&) = delete;
    ExpoDistrict& operator=(const ExpoDistrict&) = delete;

    void render(GLint modelLocation, GLint colorLocation, GLint useObjectColorLocation,
                GLint grassLocation, GLint textureLocation, GLint repeatLocation,
                GLuint stoneTexture, GLint glassLocation, GLint facadeLocation) const;
    void renderCanopyGlass(GLint modelLocation, GLint colorLocation,
                           GLint glassLocation, const Mat4& view) const;

private:
    struct Range { std::size_t first = 0; GLsizei count = 0; };
    void draw(const Range& range) const;
    Mat4 model;
    NurAlem nurAlem{center};
    ExpoPavilion pavilionNE{center,0}, pavilionNW{center,1};
    ExpoPavilion pavilionSW{center,2}, pavilionSE{center,3};
    GLuint vao = 0, vbo = 0, ebo = 0, canopyGlassEbo = 0;
    struct GlassTriangle { std::array<GLuint,3> indices; Vec3 worldCenter; };
    std::vector<GlassTriangle> canopyTriangles;
    // Reused sorting buffers avoid reallocating triangle arrays each frame.
    mutable std::vector<std::size_t> canopyOrder;
    mutable std::vector<GLuint> canopySortedIndices;
    Range lawn, paving, borders, podRoof, podGlass, podTrim, podSkylights;
    Range canopyPrimary, canopySecondary, canopyGlass;
};
