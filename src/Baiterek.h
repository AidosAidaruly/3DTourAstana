#pragma once

#include "Math3D.h"
#include "Mesh.h"

#include <array>

class Baiterek
{
public:
    Baiterek();

    // Uses the scene's active solid-color shader and shared meshes.
    void render(const Mesh& mesh, GLint modelLocation, GLint objectColorLocation) const;

private:
    static constexpr int supportCount = 8;
    std::array<Mat4, 3 + supportCount * 2> partModels{};

    // Radius 6, centered on the tower axis, with the top at y = 43.5.
    Mat4 sphereModel = modelMatrix({0, 37.5f, 0}, 0, {6, 6, 6});
};
