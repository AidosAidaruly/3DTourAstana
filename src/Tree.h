#pragma once

#include "Math3D.h"
#include "Mesh.h"

#include <array>

class Tree
{
public:
    explicit Tree(Vec3 position, float scale = 1.0f);

    // Uses the scene's active solid-color shader and shared meshes.
    void render(const Mesh& mesh, GLint modelLocation, GLint objectColorLocation) const;

private:
    Mat4 trunkModel;
    std::array<Mat4, 4> foliageModels;
};
