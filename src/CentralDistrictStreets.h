#pragma once
#include "SecondaryStreets.h"

namespace CentralDistrictStreets
{
    // Reuses CityRoadNetwork's static material batches and emission interface.
    void generate(const SecondaryStreets::EmitPolygon& polygon,const SecondaryStreets::EmitQuad& quad);
}
