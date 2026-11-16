#pragma once

#include "Bench.h"
#include "Math3D.h"
#include "Mesh.h"
#include "StreetLight.h"

#include <vector>

// Reuses the shared furniture models around Khan Shatyr without adding lights.
class KhanDistrictFurniture
{
public:
    explicit KhanDistrictFurniture(Vec3 center);

    void render(const Mesh& mesh, GLint modelLocation,
                GLint objectColorLocation) const;

private:
    struct LitterBin
    {
        Mat4 body;
        Mat4 lid;
        Mat4 opening;
    };

    std::vector<StreetLight> streetLights;
    std::vector<Bench> benches;
    std::vector<LitterBin> bins;
};
