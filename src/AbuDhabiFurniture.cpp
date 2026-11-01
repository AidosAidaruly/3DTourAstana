#include "AbuDhabiFurniture.h"

#include <array>

namespace
{
    Mat4 surface(float minX, float maxX, float minZ, float maxZ, float top)
    {
        return modelMatrix(
            {(minX + maxX) * 0.5f, top * 0.5f,
             (minZ + maxZ) * 0.5f},
            0, {maxX - minX, top, maxZ - minZ}
        );
    }
}

AbuDhabiFurniture::AbuDhabiFurniture()
{
    roadLights.reserve(12);
    pedestrianLights.reserve(6);
    benches.reserve(6);
    bins.reserve(4);

    // Six road lights on the west sidewalk and six on the north sidewalk.
    // The west row stops before the side parking driveway at z = 162..168.
    for (float z : {44.0f, 68.0f, 92.0f, 116.0f, 140.0f, 154.0f})
    {
        roadLights.emplace_back(Vec3{-207.0f, 0.06f, z}, -90.0f);
    }

    for (float x : {-191.0f, -167.0f, -143.0f,
                    -119.0f, -95.0f, -71.0f})
    {
        roadLights.emplace_back(Vec3{x, 0.06f, 187.0f}, 0.0f);
    }

    // Smaller unlit fixtures mark the forecourt's edges without occupying
    // the central pedestrian route to the entrance steps.
    for (float x : {-150.0f, -96.0f})
    {
        for (float z : {48.0f, 63.0f, 81.0f})
        {
            pedestrianLights.emplace_back(Vec3{x, 0.05f, z},
                                          x < -123.0f ? 90.0f : -90.0f,
                                          0.72f);
        }
    }

    // The benches face the two planted areas. Their existing 2.4-meter seats
    // stay fully on the approved paved forecourt.
    for (float z : {55.0f, 69.0f, 76.0f})
    {
        benches.emplace_back(Vec3{-146.5f, 0.05f, z}, 90.0f);
        benches.emplace_back(Vec3{-99.5f, 0.05f, z}, -90.0f);
    }

    for (float x : {-148.5f, -97.5f})
    {
        for (float z : {58.0f, 72.0f})
        {
            Vec3 position{x, 0.05f, z};
            bins.push_back({
                modelMatrix(position + Vec3{0, 0.42f, 0}, 0,
                            {0.42f, 0.84f, 0.42f}),
                modelMatrix(position + Vec3{0, 0.88f, 0}, 0,
                            {0.50f, 0.10f, 0.50f}),
                modelMatrix(position + Vec3{0, 0.941f, 0}, 0,
                            {0.28f, 0.015f, 0.28f})
            });
        }
    }

    // Six 2.5 x 5-meter bays share a 6-meter aisle on the west side of the
    // block. A 6-meter apron crosses the existing sidewalk at its curb opening.
    parkingAsphalt.push_back(surface(-224.0f, -213.0f, 157.5f, 172.5f, 0.055f));
    parkingAsphalt.push_back(surface(-213.0f, -209.0f, 162.0f, 168.0f, 0.055f));
    parkingAsphalt.push_back(surface(-209.0f, -205.0f, 162.0f, 168.0f, 0.075f));

    for (int boundary = 0; boundary <= 6; ++boundary)
    {
        float z = 157.5f + boundary * 2.5f;
        parkingMarkings.push_back(surface(
            -223.9f, -219.0f, z - 0.04f, z + 0.04f, 0.061f
        ));
    }

    parkingCurbs.push_back(surface(-224.30f, -224.0f,
                                   157.5f, 172.5f, 0.11f));
    parkingCurbs.push_back(surface(-224.0f, -213.0f,
                                   157.25f, 157.5f, 0.11f));
    parkingCurbs.push_back(surface(-224.0f, -213.0f,
                                   172.5f, 172.75f, 0.11f));
}

void AbuDhabiFurniture::render(const Mesh& mesh, GLint modelLocation,
                               GLint objectColorLocation,
                               GLint useAsphaltLocation) const
{
    glUniform1i(useAsphaltLocation, GL_TRUE);
    glUniform3f(objectColorLocation, 0.28f, 0.30f, 0.31f);

    for (const Mat4& surfaceModel : parkingAsphalt)
    {
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, surfaceModel.data());
        mesh.drawCube();
    }

    glUniform1i(useAsphaltLocation, GL_FALSE);
    glUniform3f(objectColorLocation, 0.85f, 0.86f, 0.83f);

    for (const Mat4& marking : parkingMarkings)
    {
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, marking.data());
        mesh.drawCube();
    }

    glUniform3f(objectColorLocation, 0.73f, 0.74f, 0.72f);

    for (const Mat4& curb : parkingCurbs)
    {
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, curb.data());
        mesh.drawCube();
    }

    for (const StreetLight& light : roadLights)
    {
        light.render(mesh, modelLocation, objectColorLocation);
    }

    for (const StreetLight& light : pedestrianLights)
    {
        light.render(mesh, modelLocation, objectColorLocation);
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
