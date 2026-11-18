#pragma once

#include "Math3D.h"
#include "Mesh.h"
#include "Tree.h"

#include <glad/glad.h>

#include <cstddef>
#include <vector>

// Reusable trees and batched low planting for the Khan Shatyr district.
class KhanDistrictLandscaping
{
public:
    explicit KhanDistrictLandscaping(Vec3 position);
    ~KhanDistrictLandscaping();

    KhanDistrictLandscaping(const KhanDistrictLandscaping&) = delete;
    KhanDistrictLandscaping& operator=(const KhanDistrictLandscaping&) = delete;

    void render(const Mesh& mesh, GLint modelLocation,
                GLint objectColorLocation, GLint useGrassLocation) const;
    void appendTrees(TreeModels& models)const;

private:
    struct IndexRange
    {
        std::size_t firstIndex = 0;
        GLsizei count = 0;
    };

    void drawRange(const IndexRange& range, Vec3 color,
                   GLint objectColorLocation) const;

    Mat4 model;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    IndexRange bedRange;
    IndexRange borderRange;
    IndexRange bufferRange;
    IndexRange shrubRange;
    std::vector<Tree> trees;
};
