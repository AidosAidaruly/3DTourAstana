#pragma once

#include "Math3D.h"
#include "Mesh.h"

#include <array>

class StreetLight
{
public:
    StreetLight(Vec3 position, float rotationDegrees, float scale = 1.0f);
    void render(const Mesh& mesh, GLint modelLocation, GLint objectColorLocation) const;

private:
    std::array<Mat4, 4> metalParts;
    Mat4 diffuser;
};
