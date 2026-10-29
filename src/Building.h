#pragma once

#include "Math3D.h"
#include "Mesh.h"

#include <vector>

struct WindowLayout
{
    int rows = 6;
    int frontColumns = 4;
    int sideColumns = 3;
    float verticalMargin = 0.0f;
    float widthFraction = 0.55f;
    float heightFraction = 0.5f;
};

class Building
{
public:
    Building(Vec3 position, Vec3 dimensions, Vec3 color, WindowLayout layout = {});

    // Uses the scene's active solid-color shader and shared cube mesh.
    void render(const Mesh& mesh, GLint modelLocation, GLint objectColorLocation) const;

private:
    Mat4 model;
    Vec3 color;
    std::vector<Mat4> windowModels;
};
