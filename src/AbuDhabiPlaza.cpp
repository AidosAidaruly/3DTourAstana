#include "AbuDhabiPlaza.h"

#include <array>

namespace
{
    constexpr float podiumHeight = 6.0f;

    struct SecondaryTower
    {
        float x;
        float z;
        float width;
        float depth;
        float top;
        float setbackHeight;
        float setbackInset;
        float setbackX;
        float setbackZ;
        Vec3 glassColor;
        int facadeMode;
    };

    // Offsets are relative to the site center. The broad midrise sits beside
    // the principal tower; three smaller volumes step down around them.
    constexpr std::array<SecondaryTower, 4> secondaryTowers{{
        {15.0f, -2.0f, 34.0f, 26.0f, 70.0f, 5.0f, 1.0f, 1.0f, 0.0f,
         {0.30f, 0.44f, 0.55f}, 1},
        {-47.0f, 1.0f, 20.0f, 20.0f, 39.0f, 4.0f, 1.0f, -0.5f, 0.0f,
         {0.30f, 0.42f, 0.52f}, 2},
        {-24.0f, 24.0f, 20.0f, 18.0f, 30.0f, 3.0f, 1.0f, 0.5f, 0.0f,
         {0.35f, 0.47f, 0.55f}, 3},
        {35.0f, 21.0f, 18.0f, 17.0f, 42.0f, 4.0f, 1.0f, 0.5f, 0.0f,
         {0.32f, 0.44f, 0.54f}, 4}
    }};
}

AbuDhabiPlaza::AbuDhabiPlaza(Vec3 position)
{
    glassParts.reserve(24);
    solidParts.reserve(96);

    auto addPart = [&](std::vector<Part>& parts, Vec3 localCenter,
                       Vec3 dimensions, Vec3 color, bool mainFacade = false,
                       int secondaryFacadeMode = 0)
    {
        parts.push_back({modelMatrix(position + localCenter, 0, dimensions),
                         color, mainFacade, secondaryFacadeMode});
    };

    // Connected front and rear podium wings give the five separate upper
    // volumes a single base without a featureless rectangular roof plate.
    addPart(solidParts, {-6, 0.4f, -1.5f}, {108, 0.8f, 45},
            {0.58f, 0.62f, 0.64f});
    addPart(glassParts, {-6, 3.1f, -1.5f}, {108, 4.6f, 45},
            {0.43f, 0.55f, 0.62f}, false, 5);
    addPart(solidParts, {-6, 5.7f, -1.5f}, {108, 0.6f, 45},
            {0.78f, 0.81f, 0.82f});

    addPart(solidParts, {-5, 0.4f, 30}, {100, 0.8f, 18},
            {0.58f, 0.62f, 0.64f});
    addPart(glassParts, {-5, 3.1f, 30}, {100, 4.6f, 18},
            {0.43f, 0.55f, 0.62f}, false, 5);
    addPart(solidParts, {-5, 5.7f, 30}, {100, 0.6f, 18},
            {0.78f, 0.81f, 0.82f});

    // Two storefront wings meet the podium at z = -24. The central wall sits
    // three meters behind their front edge to form a sheltered public entry.
    for (float wingX : {-20.0f, 24.0f})
    {
        addPart(glassParts, {wingX, 3.05f, -30}, {20, 4.5f, 12},
                {0.36f, 0.49f, 0.57f}, false, 5);
        addPart(solidParts, {wingX, 4.9f, -36.13f}, {20, 0.24f, 0.18f},
                {0.78f, 0.81f, 0.82f});
    }

    addPart(glassParts, {2, 3.05f, -28.5f}, {24, 4.5f, 9},
            {0.32f, 0.45f, 0.54f}, false, 5);

    // The light roof edge projects over the recessed doors; its underside is
    // a separate dark surface so the canopy has visible architectural depth.
    addPart(solidParts, {2, 5.65f, -30.75f}, {68, 0.5f, 13.5f},
            {0.80f, 0.82f, 0.82f});
    addPart(solidParts, {2, 5.32f, -30.75f}, {68, 0.16f, 13.5f},
            {0.36f, 0.42f, 0.45f});

    // Slender entrance columns and returns frame the opening without
    // narrowing the clear pedestrian route to the four door sections.
    for (float columnX : {-29.7f, -10.0f, 14.0f, 33.7f})
    {
        addPart(solidParts, {columnX, 3.02f, -36.20f},
                {0.32f, 4.44f, 0.32f}, {0.75f, 0.79f, 0.81f});
    }

    for (float sideX : {-10.0f, 14.0f})
    {
        addPart(solidParts, {sideX, 3.05f, -34.5f},
                {0.26f, 4.5f, 3.0f}, {0.73f, 0.78f, 0.80f});
    }

    constexpr int doorCount = 4;

    for (int door = 0; door < doorCount; ++door)
    {
        float doorX = 2.0f + (door - 1.5f) * 3.0f;
        addPart(glassParts, {doorX, 2.18f, -33.18f},
                {2.7f, 2.75f, 0.06f}, {0.20f, 0.34f, 0.43f});
        addPart(solidParts, {doorX + 0.93f, 1.85f, -33.24f},
                {0.05f, 0.48f, 0.06f}, {0.80f, 0.84f, 0.84f});
    }

    for (int frame = 0; frame <= doorCount; ++frame)
    {
        addPart(solidParts, {-4.0f + frame * 3.0f, 2.2f, -33.25f},
                {0.12f, 2.8f, 0.12f}, {0.78f, 0.82f, 0.83f});
    }

    addPart(solidParts, {2, 3.63f, -33.25f}, {12.2f, 0.12f, 0.12f},
            {0.78f, 0.82f, 0.83f});
    addPart(solidParts, {2, 0.83f, -33.25f}, {12.2f, 0.06f, 0.12f},
            {0.70f, 0.75f, 0.77f});
    addPart(solidParts, {2, 4.98f, -33.17f}, {24, 0.42f, 0.18f},
            {0.78f, 0.81f, 0.82f});

    // A slim fascia continues across the exposed front of the main podium.
    addPart(solidParts, {-6, 4.91f, -24.14f}, {108, 0.22f, 0.18f},
            {0.77f, 0.80f, 0.81f});

    constexpr float mainX = -20.0f;
    constexpr float mainZ = -4.0f;
    constexpr float mainWidth = 18.0f;
    constexpr float mainDepth = 23.0f;
    constexpr float plinthTop = 27.0f;
    constexpr float mainBodyTop = 119.0f;
    float mainBodyHeight = mainBodyTop - plinthTop;

    // A slightly broader lower section grounds the exceptionally slim shaft.
    addPart(glassParts,
            {mainX, (podiumHeight + plinthTop) * 0.5f, mainZ},
            {20.0f, plinthTop - podiumHeight, 25.0f},
            {0.31f, 0.47f, 0.60f}, true);
    addPart(solidParts, {mainX, plinthTop + 0.15f, mainZ},
            {20.4f, 0.3f, 25.4f}, {0.72f, 0.79f, 0.80f});

    addPart(glassParts,
            {mainX, plinthTop + mainBodyHeight * 0.5f, mainZ},
            {mainWidth, mainBodyHeight, mainDepth},
            {0.29f, 0.46f, 0.61f}, true);

    // Continuous corner rails reinforce the tall curtain-wall prism.
    for (float x : {-8.7f, 8.7f})
    {
        for (float side : {-1.0f, 1.0f})
        {
            addPart(solidParts,
                    {mainX + x, plinthTop + mainBodyHeight * 0.5f,
                     mainZ + side * (mainDepth * 0.5f + 0.09f)},
                    {0.3f, mainBodyHeight, 0.18f},
                    {0.68f, 0.76f, 0.79f});
        }
    }

    for (float z : {-11.2f, 11.2f})
    {
        for (float side : {-1.0f, 1.0f})
        {
            addPart(solidParts,
                    {mainX + side * (mainWidth * 0.5f + 0.09f),
                     plinthTop + mainBodyHeight * 0.5f, mainZ + z},
                    {0.18f, mainBodyHeight, 0.3f},
                    {0.68f, 0.76f, 0.79f});
        }
    }

    // These two light transfer bands are visible even when the fine facade
    // rhythm becomes too small to resolve from the Baiterek district.
    for (float height : {48.0f, 104.0f})
    {
        for (float side : {-1.0f, 1.0f})
        {
            addPart(solidParts,
                    {mainX, height, mainZ + side * 11.67f},
                    {18.5f, 0.7f, 0.22f}, {0.83f, 0.86f, 0.85f});
            addPart(solidParts,
                    {mainX + side * 9.17f, height, mainZ},
                    {0.22f, 0.7f, 23.3f}, {0.83f, 0.86f, 0.85f});
        }
    }

    // The shallow, framed crown surrounds a recessed upper glass volume.
    addPart(solidParts, {mainX, 119.4f, mainZ},
            {18.5f, 0.8f, 23.5f}, {0.79f, 0.83f, 0.84f});
    addPart(glassParts, {mainX, 123.5f, mainZ},
            {16.0f, 7.5f, 21.0f}, {0.24f, 0.36f, 0.45f});

    for (float x : {-8.7f, 8.7f})
    {
        for (float z : {-11.2f, 11.2f})
        {
            addPart(solidParts, {mainX + x, 123.65f, mainZ + z},
                    {0.6f, 7.7f, 0.6f}, {0.82f, 0.85f, 0.85f});
        }
    }

    // An open rectangular roof outline exposes the recessed upper glass.
    for (float side : {-1.0f, 1.0f})
    {
        addPart(solidParts, {mainX, 127.75f, mainZ + side * 11.5f},
                {18.8f, 0.5f, 0.8f}, {0.82f, 0.85f, 0.85f});
        addPart(solidParts, {mainX + side * 9.0f, 127.75f, mainZ},
                {0.8f, 0.5f, 22.2f}, {0.82f, 0.85f, 0.85f});
    }

    for (const SecondaryTower& tower : secondaryTowers)
    {
        float lowerTop = tower.top - tower.setbackHeight;
        float bodyHeight = lowerTop - podiumHeight;
        float bodyCenter = podiumHeight + bodyHeight * 0.5f;
        addPart(glassParts, {tower.x, bodyCenter, tower.z},
                {tower.width, bodyHeight, tower.depth}, tower.glassColor,
                false, tower.facadeMode);

        // A shallow ledge and offset upper volume produce distinct roof levels.
        addPart(solidParts, {tower.x, lowerTop + 0.15f, tower.z},
                {tower.width + 0.4f, 0.3f, tower.depth + 0.4f},
                {0.76f, 0.80f, 0.81f});

        float upperHeight = tower.setbackHeight - 0.6f;
        addPart(glassParts,
                {tower.x + tower.setbackX,
                 lowerTop + 0.3f + upperHeight * 0.5f,
                 tower.z + tower.setbackZ},
                {tower.width - 2.0f * tower.setbackInset, upperHeight,
                 tower.depth - 2.0f * tower.setbackInset},
                tower.glassColor, false, tower.facadeMode);

        // A dark reveal separates the upper glass from the light setback ledge.
        addPart(solidParts,
                {tower.x + tower.setbackX, lowerTop + 0.38f,
                 tower.z + tower.setbackZ},
                {tower.width - 2.0f * tower.setbackInset + 0.1f,
                 0.16f, tower.depth - 2.0f * tower.setbackInset + 0.1f},
                {0.37f, 0.45f, 0.49f});

        addPart(solidParts,
                {tower.x + tower.setbackX, tower.top - 0.2f,
                 tower.z + tower.setbackZ},
                {tower.width - 2.0f * tower.setbackInset + 0.4f,
                 0.4f, tower.depth - 2.0f * tower.setbackInset + 0.4f},
                {0.76f, 0.80f, 0.81f});

        // Larger structural fins remain visible beyond the shader's fine bays.
        for (float x : {-0.38f, 0.38f})
        {
            for (float side : {-1.0f, 1.0f})
            {
                addPart(solidParts,
                        {tower.x + x * tower.width, bodyCenter,
                         tower.z + side * (tower.depth * 0.5f + 0.07f)},
                        {0.18f, bodyHeight, 0.14f},
                        {0.70f, 0.77f, 0.79f});
            }
        }
    }
}

void AbuDhabiPlaza::render(const Mesh& mesh, GLint modelLocation,
                           GLint objectColorLocation, GLint shaftGlassLocation,
                           GLint facadeLocation, GLint secondaryFacadeLocation,
                           GLint glassMaterialLocation) const
{
    glUniform1i(shaftGlassLocation, GL_FALSE);
    glUniform1i(glassMaterialLocation, GL_TRUE);

    for (const Part& part : glassParts)
    {
        glUniform1i(facadeLocation, part.mainFacade ? GL_TRUE : GL_FALSE);
        glUniform1i(secondaryFacadeLocation, part.secondaryFacadeMode);
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, part.model.data());
        glUniform3f(objectColorLocation, part.color.x, part.color.y, part.color.z);
        mesh.drawCube();
    }

    glUniform1i(glassMaterialLocation, GL_FALSE);
    glUniform1i(secondaryFacadeLocation, 0);
    glUniform1i(facadeLocation, GL_FALSE);
    glUniform1i(shaftGlassLocation, GL_FALSE);

    for (const Part& part : solidParts)
    {
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, part.model.data());
        glUniform3f(objectColorLocation, part.color.x, part.color.y, part.color.z);
        mesh.drawCube();
    }
}
