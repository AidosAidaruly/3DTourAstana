#pragma once

#include "Math3D.h"
#include "Mesh.h"

#include <array>

class Bench
{
public:
    Bench(Vec3 position, float rotationDegrees);
    void render(const Mesh& mesh, GLint modelLocation, GLint objectColorLocation) const;

private:
    std::array<Mat4, 8> frameParts;
    std::array<Mat4, 7> woodenSlats;
};
