#pragma once

#include "Math3D.h"
#include "Mesh.h"

#include <vector>

class Baiterek
{
public:
    Baiterek();

    // Uses the scene's active solid-color shader and shared meshes.
    void render(
        const Mesh& mesh,
        GLint modelLocation,
        GLint objectColorLocation,
        GLint goldenPanelsLocation,
        GLint shaftGlassLocation
    ) const;

private:
    std::vector<Mat4> structureModels;
    std::vector<Mat4> braceModels;
    std::vector<Mat4> crownStructureModels;
    std::vector<Mat4> crownBraceModels;
    std::vector<Mat4> glassModels;
    std::vector<Mat4> platformModels;
    std::vector<Mat4> tipModels;

    // Radius 6, centered on the tower axis, with the top at y = 43.5.
    Mat4 sphereModel = modelMatrix({0, 37.5f, 0}, 0, {6, 6, 6});
};
