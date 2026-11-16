#pragma once

#include "Math3D.h"

#include <glad/glad.h>

#include <cstddef>

// Six related towers share one procedural mesh and a few material draw calls.
class KhanDistrictTowers
{
public:
    KhanDistrictTowers();
    ~KhanDistrictTowers();

    KhanDistrictTowers(const KhanDistrictTowers&) = delete;
    KhanDistrictTowers& operator=(const KhanDistrictTowers&) = delete;

    void render(GLint modelLocation, GLint objectColorLocation,
                GLint khanTowerFacadeLocation) const;

private:
    struct IndexRange
    {
        std::size_t firstIndex = 0;
        GLsizei count = 0;
    };

    void drawRange(const IndexRange& range, Vec3 color,
                   GLint objectColorLocation) const;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    IndexRange facadeRange;
    IndexRange entranceGlassRange;
    IndexRange frameRange;
    IndexRange roofRange;
    IndexRange baseRange;
};
