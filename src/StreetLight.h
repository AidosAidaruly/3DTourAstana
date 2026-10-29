#pragma once

#include "Math3D.h"
#include "Mesh.h"

#include <array>

class StreetLight
{
public:
    StreetLight(Vec3 position, float rotationDegrees);
    void render(const Mesh& mesh, GLint modelLocation, GLint objectColorLocation) const;

private:
    std::array<Mat4, 4> metalParts;
    Mat4 diffuser;
};
