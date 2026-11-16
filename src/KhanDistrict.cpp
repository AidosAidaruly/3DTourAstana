#include "KhanDistrict.h"
#include "CityRoadPlan.h"
#include "CentralDistrictPlan.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace
{
    constexpr int circleSegments = 128;
    constexpr float roadInnerRadius = 60.0f;
    constexpr float roadOuterRadius = 69.0f;
    constexpr float outerRoadInnerRadius = 132.0f;
    constexpr float outerRoadOuterRadius = 140.0f;
    constexpr float curbTop = 0.11f;
    constexpr float crossingHalfAngle = 12.0f * pi / 180.0f;
    constexpr float towerOpeningHalfAngle = 4.0f * pi / 180.0f;
    constexpr float mainCrossingHalfAngle = 3.2f * pi / 180.0f;
    using CrossingAngles = std::array<float, 6>;

    GLuint appendVertex(std::vector<float>& vertices, Vec3 position, Vec3 normal)
    {
        GLuint index = static_cast<GLuint>(vertices.size() / 8);
        vertices.insert(vertices.end(), {
            position.x, position.y, position.z,
            normal.x, normal.y, normal.z,
            position.x / 12.0f, position.z / 12.0f
        });
        return index;
    }

    float angleDifference(float a, float b)
    {
        return std::abs(std::atan2(std::sin(a - b), std::cos(a - b)));
    }

    bool crossesVehicleLink(float angle, float margin = 0.0f)
    {
        constexpr std::array<float, 3> linkAngles{{0.0f, pi, 1.5f * pi}};

        for (float linkAngle : linkAngles)
        {
            if (angleDifference(angle, linkAngle) < 4.0f * pi / 180.0f + margin)
            {
                return true;
            }
        }

        return false;
    }

    bool crossesParkingAccess(float angle, const CrossingAngles& towerAngles,
                              float margin = 0.0f)
    {
        return angleDifference(angle, towerAngles[2]) <
            3.0f * pi / 180.0f + margin ||
            angleDifference(angle, towerAngles[3]) <
            3.0f * pi / 180.0f + margin;
    }

    bool crossesMainApproach(float angle)
    {
        return angleDifference(angle, pi * 0.5f) < mainCrossingHalfAngle;
    }

    bool hasInnerRoadOpening(float angle, const CrossingAngles& towerAngles,
                             float margin = 0.0f)
    {
        if (crossesVehicleLink(angle, margin))
        {
            return true;
        }

        if (angleDifference(angle, pi * 0.5f) < crossingHalfAngle + margin)
        {
            return true;
        }

        for (float towerAngle : towerAngles)
        {
            if (angleDifference(angle, towerAngle) < towerOpeningHalfAngle + margin)
            {
                return true;
            }
        }

        return false;
    }

    void appendDisk(std::vector<float>& vertices, std::vector<GLuint>& indices,
                    float radius, float height)
    {
        GLuint center = appendVertex(vertices, {0, height, 0}, {0, 1, 0});

        for (int segment = 0; segment <= circleSegments; ++segment)
        {
            float angle = 2.0f * pi * segment / circleSegments;
            appendVertex(vertices,
                {radius * std::cos(angle), height, radius * std::sin(angle)},
                {0, 1, 0});
        }

        for (int segment = 0; segment < circleSegments; ++segment)
        {
            indices.insert(indices.end(), {
                center, center + segment + 2, center + segment + 1
            });
        }
    }

    void appendRing(std::vector<float>& vertices, std::vector<GLuint>& indices,
                    float innerRadius, float outerRadius, float height,
                    const CrossingAngles& towerAngles,
                    bool leaveCrossingsOpen = false)
    {
        for (int segment = 0; segment < circleSegments; ++segment)
        {
            float start = 2.0f * pi * segment / circleSegments;
            float end = 2.0f * pi * (segment + 1) / circleSegments;

            if (leaveCrossingsOpen && hasInnerRoadOpening(
                (start + end) * 0.5f, towerAngles))
            {
                continue;
            }

            GLuint a = appendVertex(vertices,
                {innerRadius * std::cos(start), height, innerRadius * std::sin(start)},
                {0, 1, 0});
            GLuint b = appendVertex(vertices,
                {outerRadius * std::cos(start), height, outerRadius * std::sin(start)},
                {0, 1, 0});
            GLuint c = appendVertex(vertices,
                {innerRadius * std::cos(end), height, innerRadius * std::sin(end)},
                {0, 1, 0});
            GLuint d = appendVertex(vertices,
                {outerRadius * std::cos(end), height, outerRadius * std::sin(end)},
                {0, 1, 0});
            indices.insert(indices.end(), {a, c, b, b, c, d});
        }
    }

    void appendCurbWall(std::vector<float>& vertices, std::vector<GLuint>& indices,
                        float radius, float bottom, bool inward,
                        const CrossingAngles& towerAngles)
    {
        for (int segment = 0; segment < circleSegments; ++segment)
        {
            float start = 2.0f * pi * segment / circleSegments;
            float end = 2.0f * pi * (segment + 1) / circleSegments;

            if (hasInnerRoadOpening((start + end) * 0.5f, towerAngles))
            {
                continue;
            }

            float middle = (start + end) * 0.5f;
            float direction = inward ? -1.0f : 1.0f;
            Vec3 normal{
                direction * std::cos(middle), 0,
                direction * std::sin(middle)
            };
            float x0 = radius * std::cos(start);
            float z0 = radius * std::sin(start);
            float x1 = radius * std::cos(end);
            float z1 = radius * std::sin(end);
            GLuint a = appendVertex(vertices, {x0, bottom, z0}, normal);
            GLuint b = appendVertex(vertices, {x0, curbTop, z0}, normal);
            GLuint c = appendVertex(vertices, {x1, bottom, z1}, normal);
            GLuint d = appendVertex(vertices, {x1, curbTop, z1}, normal);

            if (inward)
            {
                indices.insert(indices.end(), {a, c, b, b, c, d});
            }
            else
            {
                indices.insert(indices.end(), {a, b, c, b, d, c});
            }
        }
    }

    void appendUpQuad(std::vector<float>& vertices, std::vector<GLuint>& indices,
                      Vec3 a, Vec3 b, Vec3 c, Vec3 d)
    {
        GLuint first = appendVertex(vertices, a, {0, 1, 0});
        appendVertex(vertices, b, {0, 1, 0});
        appendVertex(vertices, c, {0, 1, 0});
        appendVertex(vertices, d, {0, 1, 0});

        if (cross(b - a, c - a).y > 0.0f)
        {
            indices.insert(indices.end(), {
                first, first + 1, first + 2,
                first, first + 2, first + 3
            });
        }
        else
        {
            indices.insert(indices.end(), {
                first, first + 2, first + 1,
                first, first + 3, first + 2
            });
        }
    }

    void appendEntranceLawn(std::vector<float>& vertices,
                            std::vector<GLuint>& indices, float side)
    {
        // Curve the boulevard's grass borders toward the outer curb of the inner road.
        constexpr int lengthSegments = 24;
        constexpr int acrossSegments = 8;
        constexpr float curbRadius = roadOuterRadius + 0.46f;

        for (int length = 0; length < lengthSegments; ++length)
        {
            for (int across = 0; across < acrossSegments; ++across)
            {
                auto point = [&](int lengthStep, int acrossStep)
                {
                    float along = static_cast<float>(lengthStep) / lengthSegments;
                    float acrossFraction =
                        static_cast<float>(acrossStep) / acrossSegments;
                    float approach = std::clamp((along - 0.72f) / 0.28f,
                                                0.0f, 1.0f);
                    approach = approach * approach * (3.0f - 2.0f * approach);
                    float innerX = 6.0f + 2.4f * approach;
                    float outerX = 20.0f + 2.0f * std::sin(pi * along);
                    float x = innerX + (outerX - innerX) * acrossFraction;
                    float endZ = std::sqrt(curbRadius * curbRadius - x * x);
                    float z = 120.0f + (endZ - 120.0f) * along;
                    return Vec3{side * x, 0.04f, z};
                };

                appendUpQuad(vertices, indices,
                             point(length, across),
                             point(length + 1, across),
                             point(length + 1, across + 1),
                             point(length, across + 1));
            }
        }
    }

    Vec3 localPoint(Vec3 site, Vec3 outward, float across, float forward,
                    float height)
    {
        Vec3 tangent{-outward.z, 0, outward.x};
        return {
            site.x + tangent.x * across - outward.x * forward,
            height,
            site.z + tangent.z * across - outward.z * forward
        };
    }

    void appendRadialStrip(std::vector<float>& vertices, std::vector<GLuint>& indices,
                           Vec3 outward, float startRadius, float endRadius,
                           float across, float width, float height)
    {
        Vec3 tangent{-outward.z, 0, outward.x};
        float left = across - width * 0.5f;
        float right = across + width * 0.5f;
        appendUpQuad(vertices, indices,
            {outward.x * startRadius + tangent.x * left, height,
             outward.z * startRadius + tangent.z * left},
            {outward.x * startRadius + tangent.x * right, height,
             outward.z * startRadius + tangent.z * right},
            {outward.x * endRadius + tangent.x * right, height,
             outward.z * endRadius + tangent.z * right},
            {outward.x * endRadius + tangent.x * left, height,
             outward.z * endRadius + tangent.z * left});
    }

    std::array<Vec3, 8> forecourtOutline(Vec3 site, Vec3 outward,
                                         float halfWidth, float halfDepth,
                                         float height)
    {
        constexpr float chamfer = 2.0f;
        return {{
            localPoint(site, outward, halfWidth - chamfer, halfDepth, height),
            localPoint(site, outward, -halfWidth + chamfer, halfDepth, height),
            localPoint(site, outward, -halfWidth, halfDepth - chamfer, height),
            localPoint(site, outward, -halfWidth, -halfDepth + chamfer, height),
            localPoint(site, outward, -halfWidth + chamfer, -halfDepth, height),
            localPoint(site, outward, halfWidth - chamfer, -halfDepth, height),
            localPoint(site, outward, halfWidth, -halfDepth + chamfer, height),
            localPoint(site, outward, halfWidth, halfDepth - chamfer, height)
        }};
    }

    void appendForecourt(std::vector<float>& vertices, std::vector<GLuint>& indices,
                         Vec3 site, Vec3 outward, float halfWidth, float halfDepth)
    {
        constexpr float pavingHeight = 0.075f;
        auto outline = forecourtOutline(
            site, outward, halfWidth, halfDepth, pavingHeight);
        Vec3 center{site.x, pavingHeight, site.z};

        for (std::size_t side = 0; side < outline.size(); ++side)
        {
            GLuint first = appendVertex(vertices, center, {0, 1, 0});
            appendVertex(vertices, outline[(side + 1) % outline.size()], {0, 1, 0});
            appendVertex(vertices, outline[side], {0, 1, 0});
            indices.insert(indices.end(), {first, first + 1, first + 2});
        }
    }

    void appendForecourtBorder(std::vector<float>& vertices,
                               std::vector<GLuint>& indices, Vec3 site,
                               Vec3 outward, float halfWidth, float halfDepth)
    {
        constexpr float borderWidth = 0.28f;
        auto outer = forecourtOutline(site, outward,
                                     halfWidth, halfDepth, 0.10f);
        auto inner = forecourtOutline(site, outward,
                                     halfWidth - borderWidth,
                                     halfDepth - borderWidth, 0.10f);

        // Leave the entrance-facing edge open where the radial route meets it.
        for (std::size_t side = 1; side < outer.size(); ++side)
        {
            std::size_t next = (side + 1) % outer.size();
            appendUpQuad(vertices, indices,
                         outer[side], outer[next], inner[next], inner[side]);
        }
    }

    void appendTangentialStrip(std::vector<float>& vertices,
                               std::vector<GLuint>& indices, Vec3 outward,
                               float radius, float fromAcross, float toAcross,
                               float radialWidth, float height)
    {
        Vec3 tangent{-outward.z, 0, outward.x};
        float inner = radius - radialWidth * 0.5f;
        float outer = radius + radialWidth * 0.5f;
        appendUpQuad(vertices, indices,
            {outward.x * inner + tangent.x * fromAcross, height,
             outward.z * inner + tangent.z * fromAcross},
            {outward.x * inner + tangent.x * toAcross, height,
             outward.z * inner + tangent.z * toAcross},
            {outward.x * outer + tangent.x * toAcross, height,
             outward.z * outer + tangent.z * toAcross},
            {outward.x * outer + tangent.x * fromAcross, height,
             outward.z * outer + tangent.z * fromAcross});
    }

    void appendArcStroke(std::vector<float>& vertices,
                         std::vector<GLuint>& indices, float radius,
                         float startAngle, float endAngle, float height)
    {
        for (int part = 0; part < 3; ++part)
        {
            float start = startAngle + (endAngle - startAngle) * part / 3.0f;
            float end = startAngle + (endAngle - startAngle) * (part + 1) / 3.0f;
            float inner = radius - 0.07f;
            float outer = radius + 0.07f;
            appendUpQuad(vertices, indices,
                {inner * std::cos(start), height, inner * std::sin(start)},
                {outer * std::cos(start), height, outer * std::sin(start)},
                {outer * std::cos(end), height, outer * std::sin(end)},
                {inner * std::cos(end), height, inner * std::sin(end)});
        }
    }

    void appendOuterCurb(std::vector<float>& vertices,
                         std::vector<GLuint>& indices, bool inner,
                         const CrossingAngles& towerAngles)
    {
        float roadEdge = inner ? outerRoadInnerRadius : outerRoadOuterRadius;
        float lowRadius = inner ? roadEdge - 0.4f : roadEdge;
        float highRadius = inner ? roadEdge : roadEdge + 0.4f;

        for (int segment = 0; segment < circleSegments; ++segment)
        {
            float start = 2.0f * pi * segment / circleSegments;
            float end = 2.0f * pi * (segment + 1) / circleSegments;
            float middle = (start + end) * 0.5f;
            bool opening = (!inner && (CityRoadPlan::khanOpening(middle) || CentralDistrictPlan::khanEastOpening(middle))) || crossesMainApproach(middle) ||
                (inner ? crossesVehicleLink(middle) :
                 crossesParkingAccess(middle, towerAngles));

            if (opening)
            {
                continue;
            }

            appendUpQuad(vertices, indices,
                {lowRadius * std::cos(start), 0.11f,
                 lowRadius * std::sin(start)},
                {highRadius * std::cos(start), 0.11f,
                 highRadius * std::sin(start)},
                {highRadius * std::cos(end), 0.11f,
                 highRadius * std::sin(end)},
                {lowRadius * std::cos(end), 0.11f,
                 lowRadius * std::sin(end)});

            float direction = inner ? 1.0f : -1.0f;
            Vec3 normal{direction * std::cos(middle), 0,
                        direction * std::sin(middle)};
            GLuint first = appendVertex(vertices,
                {roadEdge * std::cos(start), 0.04f,
                 roadEdge * std::sin(start)}, normal);
            appendVertex(vertices,
                {roadEdge * std::cos(start), 0.11f,
                 roadEdge * std::sin(start)}, normal);
            appendVertex(vertices,
                {roadEdge * std::cos(end), 0.04f,
                 roadEdge * std::sin(end)}, normal);
            appendVertex(vertices,
                {roadEdge * std::cos(end), 0.11f,
                 roadEdge * std::sin(end)}, normal);

            if (inner)
            {
                indices.insert(indices.end(), {
                    first, first + 1, first + 2,
                    first + 1, first + 3, first + 2
                });
            }
            else
            {
                indices.insert(indices.end(), {
                    first, first + 2, first + 1,
                    first + 1, first + 2, first + 3
                });
            }
        }
    }
}

KhanDistrict::KhanDistrict(Vec3 position)
    : model(modelMatrix(position, 0, {1, 1, 1}))
{
    std::vector<float> vertices;
    std::vector<GLuint> indices;
    CrossingAngles towerAngles{};

    for (std::size_t site = 0; site < towerSites.size(); ++site)
    {
        Vec3 relative = towerSites[site].position - position;
        towerAngles[site] = std::atan2(relative.z, relative.x);
    }

    auto recordRange = [&](IndexRange& range, std::size_t start)
    {
        range.firstIndex = start;
        range.count = static_cast<GLsizei>(indices.size() - start);
    };

    std::size_t start = indices.size();
    appendDisk(vertices, indices, 60.0f, 0.018f);
    recordRange(lawnRange, start);

    start = indices.size();
    appendEntranceLawn(vertices, indices, -1.0f);
    appendEntranceLawn(vertices, indices, 1.0f);
    recordRange(entranceLawnRange, start);

    start = indices.size();
    appendRing(vertices, indices, 52.0f, 59.5f, 0.045f, towerAngles);
    recordRange(pathRange, start);

    start = indices.size();
    appendRing(vertices, indices, roadInnerRadius, roadOuterRadius, 0.035f,
               towerAngles);
    recordRange(roadRange, start);

    // Short centerline strokes follow the circular road, with a clear entrance crossing.
    start = indices.size();
    for (int dash = 0; dash < 32; ++dash)
    {
        float angle = 2.0f * pi * (dash + 0.5f) / 32.0f;
        float halfLength = 0.055f;

        if (hasInnerRoadOpening(angle, towerAngles, halfLength))
        {
            continue;
        }

        // The short arc uses the same ring geometry as the road itself.
        for (int part = 0; part < 3; ++part)
        {
            float a = angle - halfLength + 2.0f * halfLength * part / 3.0f;
            float b = angle - halfLength + 2.0f * halfLength * (part + 1) / 3.0f;
            GLuint first = appendVertex(vertices,
                {64.43f * std::cos(a), 0.052f, 64.43f * std::sin(a)}, {0, 1, 0});
            appendVertex(vertices,
                {64.57f * std::cos(a), 0.052f, 64.57f * std::sin(a)}, {0, 1, 0});
            appendVertex(vertices,
                {64.43f * std::cos(b), 0.052f, 64.43f * std::sin(b)}, {0, 1, 0});
            appendVertex(vertices,
                {64.57f * std::cos(b), 0.052f, 64.57f * std::sin(b)}, {0, 1, 0});
            indices.insert(indices.end(), {
                first, first + 2, first + 1,
                first + 1, first + 2, first + 3
            });
        }
    }
    recordRange(markingRange, start);

    start = indices.size();
    appendRing(vertices, indices, 59.55f, 60.0f, curbTop,
               towerAngles, true);
    appendRing(vertices, indices, 69.0f, 69.45f, curbTop,
               towerAngles, true);
    appendCurbWall(vertices, indices, 60.0f, 0.035f, false, towerAngles);
    appendCurbWall(vertices, indices, 69.0f, 0.035f, true, towerAngles);
    recordRange(curbRange, start);

    // Each spoke joins the inner walking ring and the entrance-facing forecourt.
    start = indices.size();
    for (std::size_t site = 0; site < towerSites.size(); ++site)
    {
        const TowerSite& tower = towerSites[site];
        Vec3 relative = tower.position - position;
        float radius = std::sqrt(dot(relative, relative));
        Vec3 outward = relative * (1.0f / radius);
        float width = site < 2 ? 5.5f : (site < 4 ? 5.0f : 4.5f);
        float halfDepth = tower.depth * 0.5f + 5.0f;
        float halfWidth = tower.width * 0.5f + 5.0f;

        appendRadialStrip(vertices, indices, outward,
                          51.7f, 60.0f, 0, width, 0.075f);
        appendRadialStrip(vertices, indices, outward,
                          69.0f, radius - halfDepth, 0, width, 0.075f);
        appendForecourt(vertices, indices, relative, outward,
                        halfWidth, halfDepth);
    }
    recordRange(radialPavingRange, start);

    start = indices.size();
    for (std::size_t site = 0; site < towerSites.size(); ++site)
    {
        Vec3 relative = towerSites[site].position - position;
        float radius = std::sqrt(dot(relative, relative));
        Vec3 outward = relative * (1.0f / radius);
        float width = site < 2 ? 5.5f : (site < 4 ? 5.0f : 4.5f);
        appendRadialStrip(vertices, indices, outward,
                          60.0f, 69.0f, 0, width, 0.075f);
    }
    recordRange(crossingRange, start);

    start = indices.size();
    for (std::size_t site = 0; site < towerSites.size(); ++site)
    {
        Vec3 relative = towerSites[site].position - position;
        float radius = std::sqrt(dot(relative, relative));
        Vec3 outward = relative * (1.0f / radius);
        float width = site < 2 ? 5.5f : (site < 4 ? 5.0f : 4.5f);

        for (int stripe = 0; stripe < 5; ++stripe)
        {
            float center = 60.9f + stripe * 1.7f;
            appendRadialStrip(vertices, indices, outward,
                              center - 0.36f, center + 0.36f,
                              0, width - 0.4f, 0.087f);
        }
    }
    recordRange(crossingStripeRange, start);

    start = indices.size();
    for (std::size_t site = 0; site < towerSites.size(); ++site)
    {
        const TowerSite& tower = towerSites[site];
        Vec3 relative = tower.position - position;
        float radius = std::sqrt(dot(relative, relative));
        Vec3 outward = relative * (1.0f / radius);
        float width = site < 2 ? 5.5f : (site < 4 ? 5.0f : 4.5f);
        float halfDepth = tower.depth * 0.5f + 5.0f;
        float halfWidth = tower.width * 0.5f + 5.0f;

        for (float side : {-1.0f, 1.0f})
        {
            float across = side * (width * 0.5f + 0.12f);
            appendRadialStrip(vertices, indices, outward,
                              51.7f, 60.0f, across, 0.18f, 0.10f);
            appendRadialStrip(vertices, indices, outward,
                              69.0f, radius - halfDepth,
                              across, 0.18f, 0.10f);
        }

        appendForecourtBorder(vertices, indices, relative, outward,
                              halfWidth, halfDepth);
    }
    recordRange(edgeRange, start);

    // A second, wider loop serves the towers without crossing their pedestrian spokes.
    start = indices.size();
    appendRing(vertices, indices, outerRoadInnerRadius,
               outerRoadOuterRadius, 0.04f, towerAngles);
    recordRange(outerRoadRange, start);

    // A zebra crossing keeps the asphalt visible and opens both curbs on +Z.
    start = indices.size();
    Vec3 entranceDirection{0, 0, 1};
    for (int stripe = 0; stripe < 5; ++stripe)
    {
        float centerRadius = 132.8f + stripe * 1.6f;
        appendRadialStrip(vertices, indices, entranceDirection,
                          centerRadius - 0.45f, centerRadius + 0.45f,
                          0, 11.4f, 0.07f);
    }
    recordRange(mainCrossingRange, start);

    start = indices.size();
    for (float angle : {0.0f, pi, 1.5f * pi})
    {
        Vec3 outward{std::cos(angle), 0, std::sin(angle)};
        appendRadialStrip(vertices, indices, outward,
                          68.7f, 132.3f, 0, 7.0f, 0.06f);
    }
    recordRange(vehicleLinkRange, start);

    // Two mirrored, single-row parking courts sit outside the outer loop.
    start = indices.size();
    for (std::size_t site : {2u, 3u})
    {
        float angle = towerAngles[site];
        Vec3 outward{std::cos(angle), 0, std::sin(angle)};
        appendRadialStrip(vertices, indices, outward,
                          145.0f, 157.0f, 0, 26.0f, 0.045f);
        appendRadialStrip(vertices, indices, outward,
                          139.7f, 145.3f, 0, 7.0f, 0.06f);
    }
    recordRange(parkingRange, start);

    start = indices.size();
    for (int dash = 0; dash < 48; ++dash)
    {
        float angle = 2.0f * pi * (dash + 0.5f) / 48.0f;
        constexpr float halfLength = 0.028f;

        if (crossesVehicleLink(angle, halfLength) ||
            crossesParkingAccess(angle, towerAngles, halfLength))
        {
            continue;
        }

        appendArcStroke(vertices, indices, 136.0f,
                        angle - halfLength, angle + halfLength, 0.065f);
    }

    for (float angle : {0.0f, pi, 1.5f * pi})
    {
        Vec3 outward{std::cos(angle), 0, std::sin(angle)};

        for (int dash = 0; dash < 5; ++dash)
        {
            float startRadius = 76.0f + dash * 10.0f;
            appendRadialStrip(vertices, indices, outward,
                              startRadius, startRadius + 4.0f,
                              0, 0.14f, 0.075f);
        }
    }
    recordRange(vehicleMarkingRange, start);

    start = indices.size();
    for (std::size_t site : {2u, 3u})
    {
        float angle = towerAngles[site];
        Vec3 outward{std::cos(angle), 0, std::sin(angle)};

        for (int bay = 0; bay <= 10; ++bay)
        {
            float across = -12.5f + bay * 2.5f;
            appendRadialStrip(vertices, indices, outward,
                              152.0f, 157.0f, across, 0.08f, 0.075f);
        }

        appendTangentialStrip(vertices, indices, outward,
                              152.0f, -12.5f, 12.5f, 0.08f, 0.075f);
        appendTangentialStrip(vertices, indices, outward,
                              157.0f, -12.5f, 12.5f, 0.08f, 0.075f);
    }
    recordRange(parkingMarkingRange, start);

    start = indices.size();
    appendOuterCurb(vertices, indices, true, towerAngles);
    appendOuterCurb(vertices, indices, false, towerAngles);
    recordRange(outerCurbRange, start);

    start = indices.size();
    for (std::size_t site : {2u, 3u})
    {
        float angle = towerAngles[site];
        Vec3 outward{std::cos(angle), 0, std::sin(angle)};

        for (float side : {-13.0f, 13.0f})
        {
            appendRadialStrip(vertices, indices, outward,
                              145.0f, 157.0f, side, 0.35f, 0.11f);
        }

        appendTangentialStrip(vertices, indices, outward,
                              157.0f, -13.0f, 13.0f, 0.35f, 0.11f);
        appendTangentialStrip(vertices, indices, outward,
                              145.0f, -13.0f, -3.5f, 0.35f, 0.11f);
        appendTangentialStrip(vertices, indices, outward,
                              145.0f, 3.5f, 13.0f, 0.35f, 0.11f);
    }
    recordRange(parkingCurbRange, start);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                 vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint),
                 indices.data(), GL_STATIC_DRAW);
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

KhanDistrict::~KhanDistrict()
{
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void KhanDistrict::drawRange(const IndexRange& range, Vec3 color,
                             GLint objectColorLocation) const
{
    glUniform3f(objectColorLocation, color.x, color.y, color.z);
    glDrawElements(GL_TRIANGLES, range.count, GL_UNSIGNED_INT,
                   reinterpret_cast<void*>(range.firstIndex * sizeof(GLuint)));
}

void KhanDistrict::render(GLint modelLocation, GLint objectColorLocation,
                          GLint useGrassLocation, GLint useTextureLocation) const
{
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model.data());
    glBindVertexArray(vao);
    glUniform1i(useTextureLocation, GL_FALSE);

    glUniform1i(useGrassLocation, GL_TRUE);
    drawRange(lawnRange, {0.38f, 0.51f, 0.34f}, objectColorLocation);
    drawRange(entranceLawnRange, {0.35f, 0.48f, 0.33f}, objectColorLocation);
    glUniform1i(useGrassLocation, GL_FALSE);

    drawRange(pathRange, {0.70f, 0.70f, 0.67f}, objectColorLocation);
    drawRange(roadRange, {0.27f, 0.29f, 0.30f}, objectColorLocation);
    drawRange(markingRange, {0.79f, 0.80f, 0.76f}, objectColorLocation);
    drawRange(curbRange, {0.76f, 0.77f, 0.74f}, objectColorLocation);

    glUniform1i(useTextureLocation, GL_TRUE);
    drawRange(radialPavingRange, {0.80f, 0.80f, 0.78f}, objectColorLocation);
    glUniform1i(useTextureLocation, GL_FALSE);

    drawRange(crossingRange, {0.46f, 0.48f, 0.48f}, objectColorLocation);
    drawRange(crossingStripeRange, {0.87f, 0.88f, 0.86f}, objectColorLocation);
    drawRange(edgeRange, {0.74f, 0.75f, 0.73f}, objectColorLocation);

    drawRange(outerRoadRange, {0.25f, 0.27f, 0.28f}, objectColorLocation);
    drawRange(mainCrossingRange, {0.87f, 0.88f, 0.85f}, objectColorLocation);
    drawRange(vehicleLinkRange, {0.26f, 0.28f, 0.29f}, objectColorLocation);
    drawRange(parkingRange, {0.28f, 0.29f, 0.30f}, objectColorLocation);
    drawRange(vehicleMarkingRange, {0.78f, 0.79f, 0.76f}, objectColorLocation);
    drawRange(parkingMarkingRange, {0.84f, 0.85f, 0.82f}, objectColorLocation);
    drawRange(outerCurbRange, {0.75f, 0.76f, 0.73f}, objectColorLocation);
    drawRange(parkingCurbRange, {0.72f, 0.73f, 0.71f}, objectColorLocation);

    glBindVertexArray(0);
}
