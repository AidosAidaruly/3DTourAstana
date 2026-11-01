#pragma once

#include "Math3D.h"
#include "Mesh.h"
#include "Tree.h"

#include <cstddef>
#include <vector>

// Curved forecourt planting and coordinated street-tree rows.
class AbuDhabiLandscaping
{
public:
    AbuDhabiLandscaping();
    ~AbuDhabiLandscaping();

    AbuDhabiLandscaping(const AbuDhabiLandscaping&) = delete;
    AbuDhabiLandscaping& operator=(const AbuDhabiLandscaping&) = delete;

    void render(const Mesh& mesh, GLint modelLocation,
                GLint objectColorLocation, GLint useGrassLocation) const;

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
    IndexRange lawnRange;
    IndexRange lawnBorderRange;
    IndexRange soilRange;
    IndexRange soilBorderRange;
    IndexRange shrubRange;
    IndexRange stemRange;
    IndexRange redFlowerRange;
    IndexRange yellowFlowerRange;
    IndexRange orangeFlowerRange;
    std::vector<Tree> trees;
};
