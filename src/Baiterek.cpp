#include "Baiterek.h"

#include <cmath>

namespace
{
    Mat4 supportModel(Vec3 start, Vec3 end, float width)
    {
        Vec3 direction = end - start;
        float length = std::sqrt(dot(direction, direction));
        Vec3 up = normalize(direction);
        Vec3 right = normalize(cross(up, {0, 0, 1}));
        Vec3 forward = cross(right, up);
        Vec3 center = (start + end) * 0.5f;

        // Align the cube's local Y axis with the beam, then scale and translate it.
        return {
            right.x * width, right.y * width, right.z * width, 0,
            up.x * length, up.y * length, up.z * length, 0,
            forward.x * width, forward.y * width, forward.z * width, 0,
            center.x, center.y, center.z, 1
        };
    }
}

Baiterek::Baiterek()
{
    constexpr float plazaTop = 0.05f;
    constexpr float baseTop = plazaTop + 1.0f;
    constexpr float waistHeight = 24.0f;
    constexpr float supportTop = 35.0f;

    // Two square steps form a simple polygonal base resting on the plaza.
    partModels[0] = modelMatrix({0, plazaTop + 0.3f, 0}, 0, {14, 0.6f, 14});
    partModels[1] = modelMatrix({0, plazaTop + 0.8f, 0}, 0, {10, 0.4f, 10});
    partModels[2] = modelMatrix(
        {0, (baseTop + supportTop) * 0.5f, 0},
        0,
        {1.6f, supportTop - baseTop, 1.6f}
    );

    // Radial supports narrow at the waist, then flare to hold the future sphere.
    // The sphere above the frame completes the approximately 45-meter silhouette.
    for (int i = 0; i < supportCount; ++i)
    {
        float angle = 2.0f * pi * static_cast<float>(i) / supportCount;
        float x = std::cos(angle);
        float z = std::sin(angle);

        Vec3 foot{4.5f * x, baseTop, 4.5f * z};
        Vec3 waist{2.0f * x, waistHeight, 2.0f * z};
        Vec3 crown{6.0f * x, supportTop, 6.0f * z};

        partModels[3 + i * 2] = supportModel(foot, waist, 0.65f);
        partModels[4 + i * 2] = supportModel(waist, crown, 0.65f);
    }
}

void Baiterek::render(
    const Mesh& mesh,
    GLint modelLocation,
    GLint objectColorLocation
) const
{
    const float structureColor[3] = {0.90f, 0.92f, 0.94f};
    glUniform3fv(objectColorLocation, 1, structureColor);

    for (const Mat4& model : partModels)
    {
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model.data());
        mesh.drawCube();
    }

    const float sphereColor[3] = {0.95f, 0.68f, 0.16f};
    glUniform3fv(objectColorLocation, 1, sphereColor);
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, sphereModel.data());
    mesh.drawSphere();
}
