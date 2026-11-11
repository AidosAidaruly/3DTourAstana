#pragma once

#include "Math3D.h"
#include <cstdint>
#include <vector>

// Coordinates are relative to the EXPO anchor. Glass and structural members
// share the approved surface and its exact building attachment boundaries.
struct ExpoCanopySurface
{
    static constexpr int radialSegments = 20;
    static Vec3 point(float angle, float radial);
    static Vec3 normal(float angle, float radial, bool underside = false);
    static float thickness(float angle, float radial);
    static std::vector<float> angularStations();
};

struct ExpoCanopyVertex
{
    Vec3 position, normal;
    float angle, radial; // radians and [0,1], directly reusable with point().
};

struct ExpoCanopyMesh
{
    std::vector<ExpoCanopyVertex> vertices;
    std::vector<std::uint32_t> indices;
};

ExpoCanopyMesh buildExpoCanopy();

struct ExpoCanopyAssembly
{
    ExpoCanopyMesh glass, primary, secondary;
    unsigned primaryMembers = 0, secondaryMembers = 0, perimeterMembers = 0;
    unsigned glassBays = 0; // framed triangular bays, independent of glass tessellation
};

ExpoCanopyAssembly buildExpoCanopyAssembly();
