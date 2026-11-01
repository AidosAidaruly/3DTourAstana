#pragma once

#include "Bench.h"
#include "Math3D.h"
#include "Mesh.h"
#include "StreetLight.h"

#include <vector>

// Daytime street furniture and a compact side parking court for Abu Dhabi Plaza.
class AbuDhabiFurniture
{
public:
    AbuDhabiFurniture();

    void render(const Mesh& mesh, GLint modelLocation,
                GLint objectColorLocation, GLint useAsphaltLocation) const;

private:
    struct LitterBin
    {
        Mat4 body;
        Mat4 lid;
        Mat4 opening;
    };

    std::vector<StreetLight> roadLights;
    std::vector<StreetLight> pedestrianLights;
    std::vector<Bench> benches;
    std::vector<LitterBin> bins;
    std::vector<Mat4> parkingAsphalt;
    std::vector<Mat4> parkingMarkings;
    std::vector<Mat4> parkingCurbs;
};
