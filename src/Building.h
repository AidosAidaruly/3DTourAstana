#pragma once

#include "Math3D.h"
#include "Mesh.h"

#include <vector>

class Building
{
public:
    Building(Vec3 position, Vec3 dimensions, Vec3 color);

    // Uses the scene's active solid-color shader and shared cube mesh.
    void render(const Mesh& mesh, GLint modelLocation, GLint objectColorLocation) const;

private:
    Mat4 model;
    Vec3 color;
    std::vector<Mat4> windowModels;
};
