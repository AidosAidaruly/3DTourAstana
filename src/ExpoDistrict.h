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
    static constexpr float roadInnerRadius = 196, roadOuterRadius = 210;
    static constexpr float pavilionInnerRadius = 92, pavilionOuterRadius = 132;

    ExpoDistrict();
    ~ExpoDistrict();
    ExpoDistrict(const ExpoDistrict&) = delete;
    ExpoDistrict& operator=(const ExpoDistrict&) = delete;

    void render(GLint modelLocation, GLint colorLocation, GLint useObjectColorLocation,
                GLint grassLocation, GLint textureLocation, GLint repeatLocation,
                GLuint stoneTexture, GLint glassLocation, GLint facadeLocation, GLint asphaltLocation) const;
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
    // Static triangles and fixed-size scratch arrays; the EBO stays valid until
    // the camera's depth direction changes and produces a different ordering.
    struct GlassSortKey {double depth=0;std::size_t triangle=0;};
    mutable std::vector<GlassSortKey> canopyOrder;
    mutable std::vector<GLuint> canopySortedIndices;
    mutable Vec3 canopySortedDirection{};
    mutable bool canopyOrderValid=false;
    Range paving, podRoof, podGlass, podTrim, podSkylights;
    Range canopyPrimary, canopySecondary, canopyGlass;
    Range roadAsphalt, roadSidewalks, roadEdging, roadPaint;
    Range landscapePaving, landscapeEdging, landscapeBeds;
    std::array<Range,11> parkScenery;
};
