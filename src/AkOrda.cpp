#include "AkOrda.h"

#include <array>
#include <cmath>
#include <vector>

namespace
{
    constexpr int facadeSegments = 24;
    constexpr int domeSegments = 48;
    constexpr int domeBands = 12;
    constexpr int spireSegments = 12;
    constexpr float entranceZ = -26.0f;
    constexpr float domeZ = 2.0f;

    void appendVertex(std::vector<float>& vertices, Vec3 point, Vec3 normal,
                      float u = 0.0f, float v = 0.0f)
    {
        vertices.insert(vertices.end(), {
            point.x, point.y, point.z,
            normal.x, normal.y, normal.z,
            u, v
        });
    }

    void appendTriangle(std::vector<float>& vertices,
                        Vec3 a, Vec3 b, Vec3 c,
                        Vec3 normalA, Vec3 normalB, Vec3 normalC)
    {
        appendVertex(vertices, a, normalA);
        appendVertex(vertices, b, normalB);
        appendVertex(vertices, c, normalC);
    }

    Vec3 entrancePoint(float radius, float angle, float height)
    {
        return {radius * std::cos(angle), height,
                entranceZ - radius * std::sin(angle)};
    }

    void appendCurvedWall(std::vector<float>& vertices, float radius,
                          float bottom, float top, float firstAngle,
                          float lastAngle, int segments)
    {
        for (int segment = 0; segment < segments; ++segment)
        {
            float start = firstAngle + (lastAngle - firstAngle) * segment / segments;
            float end = firstAngle + (lastAngle - firstAngle) * (segment + 1) / segments;
            Vec3 normalStart{std::cos(start), 0, -std::sin(start)};
            Vec3 normalEnd{std::cos(end), 0, -std::sin(end)};
            Vec3 lowStart = entrancePoint(radius, start, bottom);
            Vec3 lowEnd = entrancePoint(radius, end, bottom);
            Vec3 highStart = entrancePoint(radius, start, top);
            Vec3 highEnd = entrancePoint(radius, end, top);

            appendTriangle(vertices, lowStart, lowEnd, highStart,
                           normalStart, normalEnd, normalStart);
            appendTriangle(vertices, lowEnd, highEnd, highStart,
                           normalEnd, normalEnd, normalStart);
        }
    }

    void appendHalfDisk(std::vector<float>& vertices, float radius, float y)
    {
        Vec3 center{0, y, entranceZ};
        Vec3 up{0, 1, 0};

        for (int segment = 0; segment < facadeSegments; ++segment)
        {
            float start = pi * segment / facadeSegments;
            float end = pi * (segment + 1) / facadeSegments;
            appendTriangle(vertices, center,
                           entrancePoint(radius, start, y),
                           entrancePoint(radius, end, y),
                           up, up, up);
        }
    }

    void appendHalfRing(std::vector<float>& vertices, float innerRadius,
                        float outerRadius, float y, bool faceUp = true)
    {
        Vec3 normal{0, faceUp ? 1.0f : -1.0f, 0};

        for (int segment = 0; segment < facadeSegments; ++segment)
        {
            float start = pi * segment / facadeSegments;
            float end = pi * (segment + 1) / facadeSegments;
            Vec3 innerStart = entrancePoint(innerRadius, start, y);
            Vec3 innerEnd = entrancePoint(innerRadius, end, y);
            Vec3 outerStart = entrancePoint(outerRadius, start, y);
            Vec3 outerEnd = entrancePoint(outerRadius, end, y);
            if (faceUp)
            {
                appendTriangle(vertices, innerStart, outerStart, outerEnd,
                               normal, normal, normal);
                appendTriangle(vertices, innerStart, outerEnd, innerEnd,
                               normal, normal, normal);
            }
            else
            {
                appendTriangle(vertices, innerStart, outerEnd, outerStart,
                               normal, normal, normal);
                appendTriangle(vertices, innerStart, innerEnd, outerEnd,
                               normal, normal, normal);
            }
        }
    }

    void appendCylinder(std::vector<float>& vertices, float radius,
                        float bottom, float top, int segments,
                        float centerX = 0.0f, float centerZ = domeZ)
    {
        Vec3 up{0, 1, 0};

        for (int segment = 0; segment < segments; ++segment)
        {
            float start = 2.0f * pi * segment / segments;
            float end = 2.0f * pi * (segment + 1) / segments;
            Vec3 normalStart{std::cos(start), 0, std::sin(start)};
            Vec3 normalEnd{std::cos(end), 0, std::sin(end)};
            Vec3 lowStart{centerX + radius * normalStart.x, bottom,
                          centerZ + radius * normalStart.z};
            Vec3 lowEnd{centerX + radius * normalEnd.x, bottom,
                        centerZ + radius * normalEnd.z};
            Vec3 highStart{lowStart.x, top, lowStart.z};
            Vec3 highEnd{lowEnd.x, top, lowEnd.z};

            appendTriangle(vertices, lowStart, highStart, lowEnd,
                           normalStart, normalStart, normalEnd);
            appendTriangle(vertices, lowEnd, highStart, highEnd,
                           normalEnd, normalStart, normalEnd);
            appendTriangle(vertices, {centerX, top, centerZ}, highEnd, highStart,
                           up, up, up);
        }
    }

    Vec3 domePoint(float angle, float latitude, float radius)
    {
        float horizontal = radius * std::cos(latitude);
        return {horizontal * std::cos(angle),
                29.4f + radius * std::sin(latitude),
                domeZ + horizontal * std::sin(angle)};
    }

    Vec3 domeNormal(float angle, float latitude)
    {
        return {std::cos(latitude) * std::cos(angle),
                std::sin(latitude),
                std::cos(latitude) * std::sin(angle)};
    }

    void appendDome(std::vector<float>& vertices)
    {
        constexpr float radius = 11.5f;

        for (int band = 0; band < domeBands - 1; ++band)
        {
            float lowLatitude = (pi * 0.5f) * band / domeBands;
            float highLatitude = (pi * 0.5f) * (band + 1) / domeBands;

            for (int segment = 0; segment < domeSegments; ++segment)
            {
                float start = 2.0f * pi * segment / domeSegments;
                float end = 2.0f * pi * (segment + 1) / domeSegments;
                Vec3 lowStart = domePoint(start, lowLatitude, radius);
                Vec3 lowEnd = domePoint(end, lowLatitude, radius);
                Vec3 highStart = domePoint(start, highLatitude, radius);
                Vec3 highEnd = domePoint(end, highLatitude, radius);
                Vec3 lowStartNormal = domeNormal(start, lowLatitude);
                Vec3 lowEndNormal = domeNormal(end, lowLatitude);
                Vec3 highStartNormal = domeNormal(start, highLatitude);
                Vec3 highEndNormal = domeNormal(end, highLatitude);

                appendTriangle(vertices, lowStart, highStart, lowEnd,
                               lowStartNormal, highStartNormal, lowEndNormal);
                appendTriangle(vertices, lowEnd, highStart, highEnd,
                               lowEndNormal, highStartNormal, highEndNormal);
            }
        }

        float lastLatitude = (pi * 0.5f) * (domeBands - 1) / domeBands;
        Vec3 apex{0, 40.9f, domeZ};

        for (int segment = 0; segment < domeSegments; ++segment)
        {
            float start = 2.0f * pi * segment / domeSegments;
            float end = 2.0f * pi * (segment + 1) / domeSegments;
            appendTriangle(vertices,
                           domePoint(start, lastLatitude, radius),
                           apex,
                           domePoint(end, lastLatitude, radius),
                           domeNormal(start, lastLatitude),
                           {0, 1, 0},
                           domeNormal(end, lastLatitude));
        }
    }

    void appendDomeRibs(std::vector<float>& vertices)
    {
        constexpr int ribCount = 12;
        constexpr int ribBands = 20;
        constexpr float surfaceRadius = 11.58f;
        constexpr float halfWidth = 0.009f;
        constexpr float firstLatitude = 0.015f;
        constexpr float lastLatitude = 1.54f;

        for (int rib = 0; rib < ribCount; ++rib)
        {
            float angle = 2.0f * pi * rib / ribCount;
            float left = angle - halfWidth;
            float right = angle + halfWidth;

            for (int band = 0; band < ribBands; ++band)
            {
                float low = firstLatitude +
                    (lastLatitude - firstLatitude) * band / ribBands;
                float high = firstLatitude +
                    (lastLatitude - firstLatitude) * (band + 1) / ribBands;
                Vec3 lowLeft = domePoint(left, low, surfaceRadius);
                Vec3 lowRight = domePoint(right, low, surfaceRadius);
                Vec3 highLeft = domePoint(left, high, surfaceRadius);
                Vec3 highRight = domePoint(right, high, surfaceRadius);
                Vec3 lowLeftNormal = domeNormal(left, low);
                Vec3 lowRightNormal = domeNormal(right, low);
                Vec3 highLeftNormal = domeNormal(left, high);
                Vec3 highRightNormal = domeNormal(right, high);

                appendTriangle(vertices, lowLeft, highLeft, lowRight,
                               lowLeftNormal, highLeftNormal, lowRightNormal);
                appendTriangle(vertices, lowRight, highLeft, highRight,
                               lowRightNormal, highLeftNormal, highRightNormal);
            }
        }
    }

    void appendTaperedSpire(std::vector<float>& vertices,
                            float bottom, float top,
                            float bottomRadius, float topRadius)
    {
        float slope = (bottomRadius - topRadius) / (top - bottom);

        for (int segment = 0; segment < spireSegments; ++segment)
        {
            float start = 2.0f * pi * segment / spireSegments;
            float end = 2.0f * pi * (segment + 1) / spireSegments;
            Vec3 normalStart = normalize(Vec3{
                std::cos(start), slope, std::sin(start)
            });
            Vec3 normalEnd = normalize(Vec3{
                std::cos(end), slope, std::sin(end)
            });
            Vec3 lowStart{bottomRadius * std::cos(start), bottom,
                          domeZ + bottomRadius * std::sin(start)};
            Vec3 lowEnd{bottomRadius * std::cos(end), bottom,
                        domeZ + bottomRadius * std::sin(end)};
            Vec3 highStart{topRadius * std::cos(start), top,
                           domeZ + topRadius * std::sin(start)};
            Vec3 highEnd{topRadius * std::cos(end), top,
                         domeZ + topRadius * std::sin(end)};

            appendTriangle(vertices, lowStart, highStart, lowEnd,
                           normalStart, normalStart, normalEnd);
            appendTriangle(vertices, lowEnd, highStart, highEnd,
                           normalEnd, normalStart, normalEnd);
        }
    }
}

AkOrda::AkOrda(Vec3 position)
    : landmarkModel(modelMatrix(position, 0, {1, 1, 1})),
      finialModel(modelMatrix(position + Vec3{0, 52.8f, domeZ},
                              0, {0.55f, 0.55f, 0.55f}))
{
    auto addStonePart = [&](Vec3 center, Vec3 size, Vec3 color)
    {
        stoneParts.push_back({modelMatrix(position + center, 0, size), color, true});
    };
    auto addFacadePart = [&](Vec3 center, Vec3 size, Vec3 color,
                             bool stoneMaterial = true)
    {
        facadeParts.push_back({modelMatrix(position + center, 0, size),
                               color, stoneMaterial});
    };

    const Vec3 ivory{0.97f, 0.96f, 0.93f};
    const Vec3 lightStone{0.99f, 0.98f, 0.95f};
    const Vec3 warmStone{0.95f, 0.94f, 0.91f};
    const Vec3 revealStone{0.78f, 0.78f, 0.75f};
    const Vec3 glass{0.49f, 0.63f, 0.71f};
    const Vec3 doorGlass{0.27f, 0.42f, 0.50f};

    auto addRoofBorder = [&](Vec3 center, float width, float depth,
                              float height, float edge)
    {
        for (float side : {-1.0f, 1.0f})
        {
            addStonePart({center.x, center.y,
                          center.z + side * (depth - edge) * 0.5f},
                         {width, height, edge}, lightStone);
            addStonePart({center.x + side * (width - edge) * 0.5f,
                          center.y, center.z},
                         {edge, height, depth}, lightStone);
        }
    };

    auto addFramedWindow = [&](float x, float y, float wallZ,
                               float width, float height)
    {
        // The shadowed backing and long frame sides make the pane read as
        // recessed behind the classical facade line.
        addFacadePart({x, y, wallZ - 0.07f},
                      {width + 0.34f, height + 0.34f, 0.06f}, revealStone);
        addFacadePart({x, y, wallZ - 0.16f},
                      {width, height, 0.07f}, glass, false);

        float sideX = width * 0.5f + 0.10f;
        float topY = height * 0.5f + 0.10f;

        for (float side : {-1.0f, 1.0f})
        {
            addFacadePart({x + side * sideX, y, wallZ - 0.36f},
                          {0.14f, height + 0.34f, 0.54f}, lightStone);
            addFacadePart({x, y + side * topY, wallZ - 0.36f},
                          {width + 0.33f, 0.14f, 0.54f}, lightStone);
        }

        addFacadePart({x, y - height * 0.5f - 0.20f, wallZ - 0.38f},
                      {width + 0.62f, 0.18f, 0.60f}, lightStone);
    };

    // A broad plinth, lower wings and taller central block make the palace
    // read as a low, symmetric civic building from the boulevard.
    addStonePart({0, 0.6f, 0}, {132, 1.2f, 68}, lightStone);
    addStonePart({0, 11.6f, 2}, {52, 20.8f, 56}, ivory);

    for (float side : {-1.0f, 1.0f})
    {
        // Only the wing wall face recedes; the approved roof and outer
        // footprint remain at their original coordinates.
        addStonePart({side * 45.5f, 9.2f, 3.75f},
                     {39, 16, 52.5f}, ivory);
        addStonePart({side * 45.5f, 17.55f, 3.5f},
                     {39.8f, 0.7f, 54}, lightStone);
        addRoofBorder({side * 45.5f, 18.12f, 3.5f},
                      39.8f, 54.0f, 0.44f, 0.50f);
    }

    addStonePart({0, 22.4f, 2}, {54, 0.8f, 58}, lightStone);
    addStonePart({0, 24.1f, 2}, {32, 2.6f, 34}, ivory);
    addRoofBorder({0, 22.94f, 2}, 54.0f, 58.0f, 0.28f, 0.48f);
    addRoofBorder({0, 25.45f, 2}, 32.4f, 34.4f, 0.20f, 0.50f);

    // Six matching bays on each wing keep three legible window levels.
    constexpr int baysPerWing = 6;
    constexpr float bayWidth = 39.0f / baysPerWing;
    constexpr std::array<float, 3> windowHeights{{3.85f, 9.20f, 14.50f}};

    for (float side : {-1.0f, 1.0f})
    {
        for (int bay = 0; bay < baysPerWing; ++bay)
        {
            float x = side * (26.0f + (bay + 0.5f) * bayWidth);

            for (float y : windowHeights)
            {
                addFramedWindow(x, y, -22.5f, 2.25f, 3.80f);
            }
        }

        for (float y : {6.65f, 11.95f, 17.12f})
        {
            addFacadePart({side * 45.5f, y, -22.96f},
                          {39.3f, 0.42f, 0.92f}, lightStone);
        }

        addFacadePart({side * 45.5f, 16.50f, -22.96f},
                      {39.3f, 0.20f, 0.88f}, ivory);
    }

    // The taller central block has matching stone piers beside the portico.
    for (float side : {-1.0f, 1.0f})
    {
        for (float y : {3.85f, 9.20f, 14.50f})
        {
            addFramedWindow(side * 18.5f, y, -26.0f, 2.25f, 3.80f);
        }

        for (float x : {12.2f, 24.0f})
        {
            float width = x < 13.0f ? 1.15f : 0.78f;
            addFacadePart({side * x, 11.65f, -26.35f},
                          {width, 20.2f, 0.70f}, lightStone);
        }

        // Short returns join the central stone bands to the recessed wings.
        for (float y : {6.65f, 11.95f, 17.12f})
        {
            addFacadePart({side * 25.8f, y, -24.55f},
                          {0.48f, 0.42f, 3.90f}, lightStone);
        }
    }

    for (float y : {6.65f, 11.95f, 17.12f, 21.95f})
    {
        addFacadePart({0, y, -26.44f},
                      {52.2f, 0.42f, 0.88f}, lightStone);
    }

    addFacadePart({0, 21.35f, -26.44f},
                  {52.2f, 0.20f, 0.88f}, ivory);

    // The colonnade entablatures continue into the rectangular central mass.
    addFacadePart({0, 10.10f, -25.82f},
                  {52.0f, 0.90f, 0.56f}, warmStone);
    addFacadePart({0, 18.77f, -25.74f},
                  {52.0f, 0.86f, 0.72f}, lightStone);
    addFacadePart({0, 21.65f, -25.74f},
                  {52.0f, 0.34f, 0.72f}, lightStone);

    // The recessed doors sit behind the open curved portico, not on its rim.
    addFacadePart({0, 4.20f, -36.48f},
                  {5.70f, 5.90f, 0.12f}, doorGlass, false);

    for (float side : {-1.0f, 1.0f})
    {
        addFacadePart({side * 1.38f, 3.80f, -36.60f},
                      {2.55f, 4.90f, 0.08f}, glass, false);
        addFacadePart({side * 3.15f, 4.20f, -37.34f},
                      {0.42f, 6.00f, 0.56f}, lightStone);
        addFacadePart({side * 0.20f, 3.45f, -36.82f},
                      {0.08f, 0.55f, 0.10f}, lightStone);
    }

    addFacadePart({0, 3.80f, -36.72f},
                  {0.15f, 4.90f, 0.16f}, lightStone);
    addFacadePart({0, 6.96f, -37.32f}, {6.9f, 0.45f, 0.60f}, lightStone);
    addFacadePart({0, 1.28f, -37.35f}, {6.9f, 0.16f, 0.70f}, lightStone);
    addFacadePart({0, 7.50f, -37.56f}, {0.80f, 0.56f, 0.24f}, lightStone);

    // Seven shallow risers reach the existing curved plinth without crossing
    // into the reserved ceremonial garden (which ends at world z = 325).
    constexpr int stairCount = 7;
    constexpr float stairTread = 0.50f;
    constexpr float stairRise = 0.175f;
    constexpr float stairFrontZ = -44.60f;

    for (int step = 0; step < stairCount; ++step)
    {
        float top = stairRise * (step + 1);
        float z = stairFrontZ + stairTread * (step + 0.5f);
        addFacadePart({0, top * 0.5f, z},
                      {10.8f, top, stairTread}, warmStone);
    }

    // The landing overlaps the curved base at its rear edge and rests on the
    // ground, so no grass gap or unsupported slab is visible at the entrance.
    addFacadePart({0, 0.65f, -40.30f},
                  {10.8f, 1.30f, 1.60f}, lightStone);

    std::vector<float> vertices;
    vertices.reserve(100000);
    auto recordRange = [&](VertexRange& range, std::size_t start)
    {
        range.first = static_cast<GLint>(start / 8);
        range.count = static_cast<GLsizei>(vertices.size() / 8 - start / 8);
    };

    std::size_t start = vertices.size();
    appendCurvedWall(vertices, 14.8f, 0.0f, 1.24f,
                     0.0f, pi, facadeSegments);
    appendHalfDisk(vertices, 14.8f, 1.24f);
    recordRange(curvedBaseRange, start);

    start = vertices.size();
    constexpr float openingStart = 10.0f * pi / facadeSegments;
    constexpr float openingEnd = 14.0f * pi / facadeSegments;
    appendCurvedWall(vertices, 11.6f, 1.24f, 7.2f,
                     0.0f, openingStart, 10);
    appendCurvedWall(vertices, 11.6f, 1.24f, 7.2f,
                     openingEnd, pi, 10);
    appendCurvedWall(vertices, 11.6f, 7.2f, 18.35f,
                     0.0f, pi, facadeSegments);
    recordRange(curvedFacadeRange, start);

    // Tall glazing stays behind the two open colonnade levels.
    start = vertices.size();
    constexpr std::array<int, 4> lowerWindowBays{{1, 3, 8, 10}};
    constexpr std::array<int, 6> upperWindowBays{{0, 2, 4, 7, 9, 11}};

    for (int bay : lowerWindowBays)
    {
        float first = pi * (2 * bay) / facadeSegments + 0.035f;
        float last = pi * (2 * bay + 2) / facadeSegments - 0.035f;
        appendCurvedWall(vertices, 11.72f, 2.15f, 8.45f, first, last, 2);
    }

    for (int bay : upperWindowBays)
    {
        float first = pi * (2 * bay) / facadeSegments + 0.035f;
        float last = pi * (2 * bay + 2) / facadeSegments - 0.035f;
        appendCurvedWall(vertices, 11.72f, 11.35f, 16.85f, first, last, 2);
    }

    recordRange(curvedWindowRange, start);

    start = vertices.size();

    for (float bottom : {8.90f, 17.55f})
    {
        appendCurvedWall(vertices, 11.78f, bottom, bottom + 0.34f,
                         0.0f, pi, facadeSegments);
    }

    // A continuous projecting entablature rests on the outer colonnade.
    appendHalfRing(vertices, 11.6f, 14.65f, 18.35f, false);
    appendCurvedWall(vertices, 14.65f, 18.35f, 19.20f,
                     0.0f, pi, facadeSegments);
    appendHalfDisk(vertices, 14.65f, 19.20f);
    appendCurvedWall(vertices, 14.75f, 19.02f, 19.20f,
                     0.0f, pi, facadeSegments);
    appendHalfRing(vertices, 14.65f, 14.75f, 19.20f);
    recordRange(curvedBandRange, start);

    start = vertices.size();
    // The intermediate balcony separates the lower and upper column orders.
    appendHalfRing(vertices, 11.6f, 14.65f, 9.65f, false);
    appendCurvedWall(vertices, 14.65f, 9.65f, 10.55f,
                     0.0f, pi, facadeSegments);
    appendHalfRing(vertices, 11.6f, 14.65f, 10.55f);
    appendCurvedWall(vertices, 14.75f, 10.37f, 10.55f,
                     0.0f, pi, facadeSegments);
    appendHalfRing(vertices, 14.65f, 14.75f, 10.55f);
    recordRange(porticoMiddleRange, start);

    start = vertices.size();
    constexpr std::array<float, 8> columnAngles{{
        15.0f, 35.0f, 55.0f, 73.0f,
        107.0f, 125.0f, 145.0f, 165.0f
    }};

    for (float degrees : columnAngles)
    {
        float angle = degrees * pi / 180.0f;
        float x = 14.10f * std::cos(angle);
        float z = entranceZ - 14.10f * std::sin(angle);
        appendCylinder(vertices, 0.54f, 1.24f, 1.40f, 12, x, z);
        appendCylinder(vertices, 0.43f, 1.40f, 1.58f, 12, x, z);
        appendCylinder(vertices, 0.34f, 1.58f, 9.25f, 12, x, z);
        appendCylinder(vertices, 0.42f, 9.25f, 9.37f, 12, x, z);
        appendCylinder(vertices, 0.55f, 9.37f, 9.65f, 12, x, z);
        appendCylinder(vertices, 0.49f, 10.55f, 10.73f, 12, x, z);
        appendCylinder(vertices, 0.40f, 10.73f, 10.90f, 12, x, z);
        appendCylinder(vertices, 0.30f, 10.90f, 18.02f, 12, x, z);
        appendCylinder(vertices, 0.38f, 18.02f, 18.13f, 12, x, z);
        appendCylinder(vertices, 0.51f, 18.13f, 18.35f, 12, x, z);
    }

    recordRange(porticoColumnsRange, start);

    start = vertices.size();

    for (float side : {-1.0f, 1.0f})
    {
        for (int pilaster = 0; pilaster <= baysPerWing; ++pilaster)
        {
            float x = side * (26.0f + pilaster * bayWidth);
            constexpr float wallZ = -22.94f;
            appendCylinder(vertices, 0.59f, 1.24f, 1.75f, 12, x, wallZ);
            appendCylinder(vertices, 0.44f, 1.75f, 16.42f, 12, x, wallZ);
            appendCylinder(vertices, 0.62f, 16.42f, 17.12f, 12, x, wallZ);
        }
    }

    recordRange(wingPilasterRange, start);

    start = vertices.size();
    appendCurvedWall(vertices, 11.6f, 19.20f, 21.45f,
                     0.0f, pi, facadeSegments);
    recordRange(upperCurvedWallRange, start);

    start = vertices.size();

    for (int bay : upperWindowBays)
    {
        float first = pi * (2 * bay) / facadeSegments + 0.035f;
        float last = pi * (2 * bay + 2) / facadeSegments - 0.035f;
        appendCurvedWall(vertices, 11.72f, 19.55f, 20.95f,
                         first, last, 2);
    }

    recordRange(upperWindowRange, start);

    start = vertices.size();
    appendHalfRing(vertices, 11.6f, 12.10f, 21.45f, false);
    appendCurvedWall(vertices, 12.10f, 21.45f, 21.82f,
                     0.0f, pi, facadeSegments);
    appendHalfDisk(vertices, 12.10f, 21.82f);
    recordRange(upperCorniceRange, start);

    start = vertices.size();
    appendCylinder(vertices, 12.5f, 25.4f, 29.4f, domeSegments);
    recordRange(domeDrumRange, start);

    start = vertices.size();
    appendCylinder(vertices, 12.75f, 25.4f, 25.76f, domeSegments);
    appendCylinder(vertices, 12.63f, 27.32f, 27.47f, domeSegments);
    appendCylinder(vertices, 12.75f, 29.04f, 29.4f, domeSegments);

    for (int flute = 0; flute < 16; ++flute)
    {
        float angle = 2.0f * pi * flute / 16.0f;
        float x = 12.51f * std::cos(angle);
        float z = domeZ + 12.51f * std::sin(angle);
        appendCylinder(vertices, 0.15f, 25.76f, 29.04f, 8, x, z);
    }

    recordRange(domeDrumTrimRange, start);

    start = vertices.size();
    appendDome(vertices);
    recordRange(domeRange, start);

    start = vertices.size();
    appendDomeRibs(vertices);
    recordRange(domeRibRange, start);

    start = vertices.size();
    appendCylinder(vertices, 0.55f, 40.9f, 41.35f, spireSegments);
    appendTaperedSpire(vertices, 41.35f, 52.70f, 0.36f, 0.12f);
    appendCylinder(vertices, 0.30f, 47.55f, 47.73f, spireSegments);
    appendCylinder(vertices, 0.22f, 52.12f, 52.28f, spireSegments);
    recordRange(spireRange, start);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                 vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          reinterpret_cast<void*>(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

AkOrda::~AkOrda()
{
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void AkOrda::drawRange(const VertexRange& range, Vec3 color,
                       GLint objectColorLocation) const
{
    glUniform3f(objectColorLocation, color.x, color.y, color.z);
    glDrawArrays(GL_TRIANGLES, range.first, range.count);
}

void AkOrda::render(const Mesh& mesh, GLint modelLocation,
                    GLint objectColorLocation, GLint stoneMaterialLocation,
                    GLint roofMaterialLocation) const
{
    glUniform1i(roofMaterialLocation, GL_FALSE);
    glUniform1i(stoneMaterialLocation, GL_TRUE);

    for (const Part& part : stoneParts)
    {
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, part.model.data());
        glUniform3f(objectColorLocation,
                    part.color.x, part.color.y, part.color.z);
        mesh.drawCube();
    }

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, landmarkModel.data());
    glBindVertexArray(vao);
    drawRange(curvedBaseRange, {0.98f, 0.97f, 0.94f}, objectColorLocation);
    drawRange(curvedFacadeRange, {0.97f, 0.96f, 0.93f}, objectColorLocation);
    glUniform1i(stoneMaterialLocation, GL_FALSE);
    drawRange(curvedWindowRange, {0.49f, 0.63f, 0.71f}, objectColorLocation);
    glUniform1i(stoneMaterialLocation, GL_TRUE);
    drawRange(curvedBandRange, {0.99f, 0.98f, 0.95f}, objectColorLocation);
    drawRange(porticoMiddleRange, {0.96f, 0.95f, 0.92f}, objectColorLocation);
    drawRange(porticoColumnsRange, {0.99f, 0.98f, 0.95f}, objectColorLocation);
    drawRange(wingPilasterRange, {0.98f, 0.97f, 0.94f}, objectColorLocation);
    drawRange(upperCurvedWallRange, {0.95f, 0.94f, 0.91f}, objectColorLocation);
    glUniform1i(stoneMaterialLocation, GL_FALSE);
    drawRange(upperWindowRange, {0.54f, 0.68f, 0.76f}, objectColorLocation);
    glUniform1i(stoneMaterialLocation, GL_TRUE);
    drawRange(upperCorniceRange, {0.99f, 0.98f, 0.95f}, objectColorLocation);
    glUniform1i(stoneMaterialLocation, GL_FALSE);
    drawRange(domeDrumRange, {0.91f, 0.91f, 0.88f}, objectColorLocation);
    glUniform1i(stoneMaterialLocation, GL_TRUE);
    drawRange(domeDrumTrimRange, {0.98f, 0.97f, 0.94f}, objectColorLocation);
    glUniform1i(stoneMaterialLocation, GL_FALSE);
    glUniform1i(roofMaterialLocation, GL_TRUE);
    drawRange(domeRange, {0.14f, 0.62f, 0.83f}, objectColorLocation);
    drawRange(domeRibRange, {0.88f, 0.74f, 0.42f}, objectColorLocation);
    drawRange(spireRange, {0.93f, 0.74f, 0.31f}, objectColorLocation);
    glUniform1i(roofMaterialLocation, GL_FALSE);
    glBindVertexArray(0);

    for (const Part& part : facadeParts)
    {
        glUniform1i(stoneMaterialLocation, part.stoneMaterial ? GL_TRUE : GL_FALSE);
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, part.model.data());
        glUniform3f(objectColorLocation,
                    part.color.x, part.color.y, part.color.z);
        mesh.drawCube();
    }

    glUniform1i(stoneMaterialLocation, GL_FALSE);
    glUniform1i(roofMaterialLocation, GL_TRUE);
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, finialModel.data());
    glUniform3f(objectColorLocation, 0.96f, 0.79f, 0.34f);
    mesh.drawSphere();
    glUniform1i(roofMaterialLocation, GL_FALSE);
}
