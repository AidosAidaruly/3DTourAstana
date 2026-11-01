#pragma once

#include "Math3D.h"
#include "Mesh.h"

#include <vector>

// A compact, reusable silhouette of the connected Abu Dhabi Plaza complex.
class AbuDhabiPlaza
{
public:
    explicit AbuDhabiPlaza(Vec3 position);

    void render(const Mesh& mesh, GLint modelLocation,
                GLint objectColorLocation, GLint shaftGlassLocation,
                GLint facadeLocation, GLint secondaryFacadeLocation,
                GLint glassMaterialLocation) const;

private:
    struct Part
    {
        Mat4 model;
        Vec3 color;
        bool mainFacade;
        int secondaryFacadeMode;
    };

    std::vector<Part> glassParts;
    std::vector<Part> solidParts;
};
