#pragma once
#include "Math3D.h"
#include <functional>
#include <vector>

namespace SecondaryStreets
{
    enum Material {Asphalt,Stone,Paint};
    // Callbacks append into CityRoadNetwork's existing static material batches.
    using EmitPolygon=std::function<void(int,std::vector<Vec3>,float)>;
    using EmitQuad=std::function<void(int,Vec3,Vec3,Vec3,Vec3,Vec3)>;
    void generate(const EmitPolygon& polygon,const EmitQuad& quad);
}
