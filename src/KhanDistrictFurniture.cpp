#include "KhanDistrictFurniture.h"

#include "KhanDistrict.h"

#include <cmath>

namespace
{
    Vec3 radialDirection(Vec3 point, Vec3 center)
    {
        return normalize(Vec3{point.x - center.x, 0, point.z - center.z});
    }

    float inwardRotation(Vec3 outward)
    {
        // StreetLight and Bench both face local -Z.
        return 90.0f - std::atan2(outward.z, outward.x) * 180.0f / pi;
    }
}

KhanDistrictFurniture::KhanDistrictFurniture(Vec3 center)
{
    streetLights.reserve(24);
    benches.reserve(8);
    bins.reserve(4);

    // Sixteen full-height lamps form a regular row beyond the outer road curb.
    for (int lamp = 0; lamp < 16; ++lamp)
    {
        float degrees = 11.25f + lamp * 22.5f;
        float angle = degrees * pi / 180.0f;
        Vec3 outward{std::cos(angle), 0, std::sin(angle)};
        streetLights.emplace_back(
            center + outward * 145.5f,
            inwardRotation(outward)
        );
    }

    // Smaller pedestrian lamps stand beside, rather than on, the six spokes.
    for (const KhanDistrict::TowerSite& site : KhanDistrict::towerSites)
    {
        Vec3 outward = radialDirection(site.position, center);
        float angle = std::atan2(outward.z, outward.x);
        float side = site.position.x < 0 ? 1.0f : -1.0f;
        angle += side * 12.0f * pi / 180.0f;
        Vec3 lampDirection{std::cos(angle), 0, std::sin(angle)};
        streetLights.emplace_back(
            center + lampDirection * 78.0f,
            inwardRotation(lampDirection),
            0.78f
        );
    }

    // Two additional lamps mark the east and west sides of the inner walk.
    streetLights.emplace_back(center + Vec3{49, 0.018f, 0},
                              inwardRotation({1, 0, 0}), 0.78f);
    streetLights.emplace_back(center + Vec3{-49, 0.018f, 0},
                              inwardRotation({-1, 0, 0}), 0.78f);

    // One bench per tower sits at the forecourt edge, facing Khan Shatyr.
    for (std::size_t index = 0; index < KhanDistrict::towerSites.size(); ++index)
    {
        const KhanDistrict::TowerSite& site = KhanDistrict::towerSites[index];
        Vec3 outward = radialDirection(site.position, center);
        Vec3 tangent{-outward.z, 0, outward.x};
        float side = site.position.x < 0 ? 1.0f : -1.0f;
        Vec3 forecourtCenter = center + outward * 100.0f;
        Vec3 benchPosition = forecourtCenter + tangent * (side * 8.0f);
        benchPosition.y = 0.075f;
        benches.emplace_back(benchPosition, inwardRotation(outward));

        if (index < 4)
        {
            Vec3 binPosition = forecourtCenter + tangent * (side * 10.5f);
            binPosition.y = 0.075f;
            bins.push_back({
                modelMatrix(binPosition + Vec3{0, 0.42f, 0}, 0,
                            {0.42f, 0.84f, 0.42f}),
                modelMatrix(binPosition + Vec3{0, 0.88f, 0}, 0,
                            {0.50f, 0.10f, 0.50f}),
                modelMatrix(binPosition + Vec3{0, 0.941f, 0}, 0,
                            {0.28f, 0.015f, 0.28f})
            });
        }
    }

    // A symmetric pair overlooks the landmark from the inner walking ring.
    for (float degrees : {30.0f, 150.0f})
    {
        float angle = degrees * pi / 180.0f;
        Vec3 outward{std::cos(angle), 0, std::sin(angle)};
        Vec3 position = center + outward * 56.0f;
        position.y = 0.045f;
        benches.emplace_back(position, inwardRotation(outward));
    }
}

void KhanDistrictFurniture::render(const Mesh& mesh, GLint modelLocation,
                                   GLint objectColorLocation) const
{
    for (const StreetLight& streetLight : streetLights)
    {
        streetLight.render(mesh, modelLocation, objectColorLocation);
    }

    for (const Bench& bench : benches)
    {
        bench.render(mesh, modelLocation, objectColorLocation);
    }

    for (const LitterBin& bin : bins)
    {
        glUniform3f(objectColorLocation, 0.25f, 0.29f, 0.31f);
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, bin.body.data());
        mesh.drawCube();

        glUniform3f(objectColorLocation, 0.40f, 0.43f, 0.44f);
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, bin.lid.data());
        mesh.drawCube();

        glUniform3f(objectColorLocation, 0.08f, 0.09f, 0.09f);
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, bin.opening.data());
        mesh.drawCube();
    }
}
