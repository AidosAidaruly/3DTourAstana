#pragma once

#include "Math3D.h"
#include "Mesh.h"

#include <array>

class Tree
{
public:
    // Shared crown description for individual trees and batched landscaping.
    static constexpr std::array<Vec3, 4> foliageOffsets{{
        {-1.0f, 4.6f, 0}, {1.0f, 4.9f, 0.4f},
        {0, 5.2f, -1.0f}, {0.1f, 6.3f, 0}
    }};
    static constexpr std::array<float, 4> foliageRadii{{1.8f, 1.9f, 1.75f, 1.7f}};

    explicit Tree(Vec3 position, float scale = 1.0f,
                  float foliageVariation = 0.0f);

    // Uses the scene's active solid-color shader and shared meshes.
    void render(const Mesh& mesh, GLint modelLocation, GLint objectColorLocation) const;

private:
    Mat4 trunkModel;
    std::array<Mat4, 4> foliageModels;
    float foliageVariation = 0.0f;
};
