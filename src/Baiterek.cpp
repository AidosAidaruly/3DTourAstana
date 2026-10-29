#include "Baiterek.h"

#include <array>
#include <cmath>

namespace
{
    Vec3 radialPoint(float angle, float radius, float height)
    {
        return {radius * std::cos(angle), height, radius * std::sin(angle)};
    }

    Mat4 beamModel(Vec3 start, Vec3 end, float width)
    {
        Vec3 direction = end - start;
        float length = std::sqrt(dot(direction, direction));
        Vec3 up = normalize(direction);
        Vec3 reference = std::abs(up.z) > 0.95f ? Vec3{1, 0, 0} : Vec3{0, 0, 1};
        Vec3 right = normalize(cross(up, reference));
        Vec3 forward = cross(right, up);
        Vec3 center = (start + end) * 0.5f;

        return {
            right.x * width, right.y * width, right.z * width, 0,
            up.x * length, up.y * length, up.z * length, 0,
            forward.x * width, forward.y * width, forward.z * width, 0,
            center.x, center.y, center.z, 1
        };
    }

    Mat4 glassPanelModel(float angle, float radius, float centerHeight, float height, float width)
    {
        Vec3 radial{std::cos(angle), 0, std::sin(angle)};
        Vec3 tangent{radial.z, 0, -radial.x};

        return {
            tangent.x * width, 0, tangent.z * width, 0,
            0, height, 0, 0,
            radial.x * 0.08f, 0, radial.z * 0.08f, 0,
            radial.x * radius, centerHeight, radial.z * radius, 1
        };
    }

}

Baiterek::Baiterek()
{
    constexpr float plazaTop = 0.05f;
    constexpr float baseTop = plazaTop + 1.0f;
    constexpr int supportCount = 8;
    constexpr float angleStep = 2.0f * pi / supportCount;

    // Keep the two original base steps.
    structureModels.push_back(modelMatrix({0, plazaTop + 0.3f, 0}, 0, {14, 0.6f, 14}));
    structureModels.push_back(modelMatrix({0, plazaTop + 0.8f, 0}, 0, {10, 0.4f, 10}));

    // The enclosed central tower has twelve blue-gray glazed faces.
    constexpr int shaftSides = 12;
    constexpr float shaftRadius = 1.65f;
    constexpr float shaftStep = 2.0f * pi / shaftSides;
    float panelWidth = 2.0f * shaftRadius * std::sin(shaftStep * 0.5f) + 0.02f;

    for (int side = 0; side < shaftSides; ++side)
    {
        float angle = static_cast<float>(side) * shaftStep;
        glassModels.push_back(glassPanelModel(
            angle + shaftStep * 0.5f, shaftRadius, 15.8f, 29.5f, panelWidth
        ));
        structureModels.push_back(beamModel(
            radialPoint(angle, shaftRadius, baseTop),
            radialPoint(angle, shaftRadius, 30.6f), 0.15f
        ));

        for (float height : {8.0f, 16.0f, 24.0f, 30.5f})
        {
            braceModels.push_back(beamModel(
                radialPoint(angle, shaftRadius, height),
                radialPoint(angle + shaftStep, shaftRadius, height), 0.13f
            ));
        }
    }

    // Eight open lattice ribs rise beside the glass tower.
    const std::array<float, 5> lowerHeights{{baseTop, 8.0f, 16.0f, 23.0f, 28.0f}};
    const std::array<float, 5> lowerRadii{{4.3f, 3.5f, 2.4f, 2.5f, 3.5f}};

    for (int i = 0; i < supportCount; ++i)
    {
        float angle = static_cast<float>(i) * angleStep;
        float nextAngle = static_cast<float>(i + 1) * angleStep;

        for (std::size_t level = 0; level + 1 < lowerHeights.size(); ++level)
        {
            Vec3 lower = radialPoint(angle, lowerRadii[level], lowerHeights[level]);
            Vec3 upper = radialPoint(angle, lowerRadii[level + 1], lowerHeights[level + 1]);
            structureModels.push_back(beamModel(lower, upper, 0.48f));

            if (level > 0)
            {
                Vec3 nextLower = radialPoint(nextAngle, lowerRadii[level], lowerHeights[level]);
                Vec3 nextUpper = radialPoint(nextAngle, lowerRadii[level + 1], lowerHeights[level + 1]);
                braceModels.push_back(beamModel(lower, nextUpper, 0.15f));
                braceModels.push_back(beamModel(nextLower, upper, 0.15f));
                braceModels.push_back(beamModel(lower, nextLower, 0.18f));
            }
        }

        // Each upper assembly has two outer rails, an inner spine, and
        // regular triangular webs; branching begins below the sphere.
        const std::array<float, 5> heights{{23.0f, 27.0f, 30.0f, 33.0f, 36.0f}};
        const std::array<float, 5> radii{{2.5f, 3.4f, 4.6f, 6.7f, 8.45f}};
        std::array<Vec3, 5> left{};
        std::array<Vec3, 5> right{};
        std::array<Vec3, 5> spine{};
        std::array<Vec3, 5> rearLeft{};
        std::array<Vec3, 5> rearRight{};

        left[0] = right[0] = spine[0] = radialPoint(angle, radii[0], heights[0]);
        rearLeft[0] = rearRight[0] = spine[0];

        for (std::size_t level = 1; level < heights.size(); ++level)
        {
            left[level] = radialPoint(angle - 0.15f, radii[level], heights[level]);
            right[level] = radialPoint(angle + 0.15f, radii[level], heights[level]);
            spine[level] = radialPoint(angle, radii[level] - 0.45f, heights[level]);
            rearLeft[level] = radialPoint(angle - 0.15f, radii[level] - 0.65f, heights[level]);
            rearRight[level] = radialPoint(angle + 0.15f, radii[level] - 0.65f, heights[level]);
        }

        for (std::size_t level = 0; level + 1 < heights.size(); ++level)
        {
            crownStructureModels.push_back(beamModel(left[level], left[level + 1], 0.43f));
            crownStructureModels.push_back(beamModel(right[level], right[level + 1], 0.43f));
            crownStructureModels.push_back(beamModel(spine[level], spine[level + 1], 0.26f));
            crownStructureModels.push_back(beamModel(rearLeft[level], rearLeft[level + 1], 0.27f));
            crownStructureModels.push_back(beamModel(rearRight[level], rearRight[level + 1], 0.27f));

            if (level > 0)
            {
                crownBraceModels.push_back(beamModel(left[level], right[level + 1], 0.17f));
                crownBraceModels.push_back(beamModel(right[level], left[level + 1], 0.17f));
                crownBraceModels.push_back(beamModel(left[level], rearLeft[level + 1], 0.13f));
                crownBraceModels.push_back(beamModel(right[level], rearRight[level + 1], 0.13f));
            }

            crownBraceModels.push_back(beamModel(left[level + 1], right[level + 1], 0.20f));
            crownBraceModels.push_back(beamModel(spine[level + 1], left[level + 1], 0.15f));
            crownBraceModels.push_back(beamModel(spine[level + 1], right[level + 1], 0.15f));
            crownBraceModels.push_back(beamModel(left[level + 1], rearLeft[level + 1], 0.14f));
            crownBraceModels.push_back(beamModel(right[level + 1], rearRight[level + 1], 0.14f));
        }

        // The front and rear rail pairs converge toward one open apex.
        // Cross-braces make triangles while leaving the space between them clear.
        Vec3 apex = radialPoint(angle, 10.5f, 41.0f);
        Vec3 middleLeft = (left.back() + apex) * 0.5f;
        Vec3 middleRight = (right.back() + apex) * 0.5f;
        Vec3 middleSpine = (spine.back() + apex) * 0.5f;
        Vec3 middleRearLeft = (rearLeft.back() + apex) * 0.5f;
        Vec3 middleRearRight = (rearRight.back() + apex) * 0.5f;

        tipModels.push_back(beamModel(left.back(), middleLeft, 0.20f));
        tipModels.push_back(beamModel(right.back(), middleRight, 0.20f));
        tipModels.push_back(beamModel(spine.back(), middleSpine, 0.16f));
        tipModels.push_back(beamModel(middleLeft, apex, 0.14f));
        tipModels.push_back(beamModel(middleRight, apex, 0.14f));
        tipModels.push_back(beamModel(middleSpine, apex, 0.12f));
        tipModels.push_back(beamModel(rearLeft.back(), middleRearLeft, 0.16f));
        tipModels.push_back(beamModel(rearRight.back(), middleRearRight, 0.16f));
        tipModels.push_back(beamModel(middleRearLeft, apex, 0.11f));
        tipModels.push_back(beamModel(middleRearRight, apex, 0.11f));

        tipModels.push_back(beamModel(middleLeft, middleRight, 0.13f));
        tipModels.push_back(beamModel(left.back(), middleRight, 0.11f));
        tipModels.push_back(beamModel(right.back(), middleLeft, 0.11f));
        tipModels.push_back(beamModel(middleSpine, middleLeft, 0.11f));
        tipModels.push_back(beamModel(middleSpine, middleRight, 0.11f));
        tipModels.push_back(beamModel(middleLeft, middleRearLeft, 0.11f));
        tipModels.push_back(beamModel(middleRight, middleRearRight, 0.11f));
        tipModels.push_back(beamModel(rearLeft.back(), middleLeft, 0.10f));
        tipModels.push_back(beamModel(rearRight.back(), middleRight, 0.10f));

        braceModels.push_back(beamModel(
            radialPoint(angle, shaftRadius, 28.0f),
            radialPoint(angle, 4.7f, 30.3f), 0.24f
        ));
    }

    // Fill the spaces between the eight main ribs with a regular outer lattice.
    // Its nodes follow the same tapered profile and meet the crown roots at y = 28.
    constexpr int latticeSides = supportCount * 2;
    constexpr float latticeStep = 2.0f * pi / latticeSides;
    const std::array<float, 7> latticeHeights{{baseTop, 5.0f, 10.0f, 15.0f, 20.0f, 24.0f, 28.0f}};
    std::array<float, latticeHeights.size()> latticeRadii{};

    for (std::size_t level = 0; level < latticeHeights.size(); ++level)
    {
        float height = latticeHeights[level];
        std::size_t band = 0;

        while (band + 2 < lowerHeights.size() && height > lowerHeights[band + 1])
        {
            ++band;
        }

        float fraction = (height - lowerHeights[band]) /
            (lowerHeights[band + 1] - lowerHeights[band]);
        latticeRadii[level] = lowerRadii[band] +
            (lowerRadii[band + 1] - lowerRadii[band]) * fraction;
    }

    for (int side = 0; side < latticeSides; ++side)
    {
        float angle = static_cast<float>(side) * latticeStep;
        float nextAngle = static_cast<float>(side + 1) * latticeStep;

        // The even positions already have major ribs; add eight intermediate ribs.
        if (side % 2 != 0)
        {
            for (std::size_t level = 0; level + 1 < latticeHeights.size(); ++level)
            {
                structureModels.push_back(beamModel(
                    radialPoint(angle, latticeRadii[level], latticeHeights[level]),
                    radialPoint(angle, latticeRadii[level + 1], latticeHeights[level + 1]),
                    0.34f
                ));
            }
        }

        for (std::size_t level = 0; level < latticeHeights.size(); ++level)
        {
            structureModels.push_back(beamModel(
                radialPoint(angle, latticeRadii[level], latticeHeights[level]),
                radialPoint(nextAngle, latticeRadii[level], latticeHeights[level]),
                0.22f
            ));
        }

        // Alternate the diagonal direction by band to create orderly triangles.
        for (std::size_t level = 0; level + 1 < latticeHeights.size(); ++level)
        {
            float lowerAngle = level % 2 == 0 ? angle : nextAngle;
            float upperAngle = level % 2 == 0 ? nextAngle : angle;
            braceModels.push_back(beamModel(
                radialPoint(lowerAngle, latticeRadii[level], latticeHeights[level]),
                radialPoint(upperAngle, latticeRadii[level + 1], latticeHeights[level + 1]),
                0.14f
            ));
        }
    }

    // Bridge the open bays between neighboring crown trusses. The lower levels
    // form a connected web; fewer members near the tips keep the sphere open.
    const std::array<float, 4> bridgeHeights{{27.0f, 30.0f, 33.0f, 36.0f}};
    const std::array<float, 4> bridgeRadii{{3.4f, 4.6f, 6.7f, 8.45f}};

    for (int i = 0; i < supportCount; ++i)
    {
        float angle = static_cast<float>(i) * angleStep;
        float nextAngle = static_cast<float>(i + 1) * angleStep;
        float middleAngle = angle + angleStep * 0.5f;
        std::array<Vec3, 4> rightNodes{};
        std::array<Vec3, 4> nextLeftNodes{};

        for (std::size_t level = 0; level < bridgeHeights.size(); ++level)
        {
            rightNodes[level] = radialPoint(
                angle + 0.15f, bridgeRadii[level], bridgeHeights[level]
            );
            nextLeftNodes[level] = radialPoint(
                nextAngle - 0.15f, bridgeRadii[level], bridgeHeights[level]
            );

            float width = level < 2 ? 0.22f : (level == 2 ? 0.18f : 0.14f);
            crownStructureModels.push_back(beamModel(
                rightNodes[level], nextLeftNodes[level], width
            ));
        }

        for (std::size_t level = 0; level + 1 < bridgeHeights.size(); ++level)
        {
            crownStructureModels.push_back(beamModel(
                rightNodes[level], nextLeftNodes[level + 1], 0.15f
            ));

            if (level < 2)
            {
                crownStructureModels.push_back(beamModel(
                    nextLeftNodes[level], rightNodes[level + 1], 0.15f
                ));
            }
        }

        // The middle node already exists on the 16-sided lower ring and the
        // outer platform ring; these struts tie both to the upper crown web.
        Vec3 lowerNode = radialPoint(middleAngle, 3.5f, 28.0f);
        Vec3 platformNode = radialPoint(middleAngle, 5.4f, 30.3f);
        crownStructureModels.push_back(beamModel(lowerNode, platformNode, 0.20f));
        crownStructureModels.push_back(beamModel(platformNode, rightNodes[2], 0.16f));
        crownStructureModels.push_back(beamModel(platformNode, nextLeftNodes[2], 0.16f));
    }

    // Concentric polygonal rails form a circular platform beneath the sphere.
    constexpr int ringSegments = 32;
    constexpr float ringStep = 2.0f * pi / ringSegments;

    for (int segment = 0; segment < ringSegments; ++segment)
    {
        float angle = static_cast<float>(segment) * ringStep;
        float nextAngle = static_cast<float>(segment + 1) * ringStep;

        for (float radius : {4.7f, 5.4f})
        {
            platformModels.push_back(beamModel(
                radialPoint(angle, radius, 30.3f),
                radialPoint(nextAngle, radius, 30.3f), 0.32f
            ));
        }

        if (segment % 2 == 0)
        {
            platformModels.push_back(beamModel(
                radialPoint(angle, 4.7f, 30.3f),
                radialPoint(angle, 5.4f, 30.3f), 0.26f
            ));
        }
    }
}

void Baiterek::render(
    const Mesh& mesh,
    GLint modelLocation,
    GLint objectColorLocation,
    GLint goldenPanelsLocation,
    GLint shaftGlassLocation
) const
{
    const float white[3] = {0.90f, 0.92f, 0.94f};
    const float silver[3] = {0.78f, 0.82f, 0.86f};
    const float crownWhite[3] = {0.98f, 0.98f, 0.99f};
    const float crownSilver[3] = {0.91f, 0.94f, 0.97f};
    const float blueGlass[3] = {0.36f, 0.54f, 0.65f};

    glUniform3fv(objectColorLocation, 1, blueGlass);
    glUniform1i(shaftGlassLocation, GL_TRUE);

    for (const Mat4& model : glassModels)
    {
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model.data());
        mesh.drawCube();
    }

    glUniform1i(shaftGlassLocation, GL_FALSE);
    glUniform3fv(objectColorLocation, 1, white);

    for (const Mat4& model : structureModels)
    {
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model.data());
        mesh.drawCube();
    }

    glUniform3fv(objectColorLocation, 1, silver);

    for (const Mat4& model : braceModels)
    {
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model.data());
        mesh.drawCube();
    }

    glUniform3fv(objectColorLocation, 1, white);

    for (const Mat4& model : platformModels)
    {
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model.data());
        mesh.drawCube();
    }

    glUniform3fv(objectColorLocation, 1, crownWhite);

    for (const Mat4& model : crownStructureModels)
    {
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model.data());
        mesh.drawCube();
    }

    glUniform3fv(objectColorLocation, 1, crownSilver);

    for (const Mat4& model : crownBraceModels)
    {
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model.data());
        mesh.drawCube();
    }

    glUniform3fv(objectColorLocation, 1, crownWhite);

    for (const Mat4& model : tipModels)
    {
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model.data());
        mesh.drawCube();
    }

    const float sphereColor[3] = {0.95f, 0.68f, 0.16f};
    glUniform3fv(objectColorLocation, 1, sphereColor);
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, sphereModel.data());
    glUniform1i(goldenPanelsLocation, GL_TRUE);
    mesh.drawSphere();
    glUniform1i(goldenPanelsLocation, GL_FALSE);
}
