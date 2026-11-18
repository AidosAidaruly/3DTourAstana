#include "KhanShatyr.h"
#include "SunShadows.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace
{
    constexpr int radialSegments = 64;
    constexpr int heightSegments = 32;
    constexpr float baseRadiusX = 37.0f;
    constexpr float baseRadiusZ = 28.0f;
    constexpr float canopyBottom = 4.2f;
    constexpr float canopyPeak = 47.0f;
    constexpr float upperRingRadius = 2.4f;
    constexpr float upperOffsetX = -6.0f;
    constexpr float profilePower = 1.23f;
    constexpr float mastBaseRadius = 0.32f;
    constexpr float mastTipRadius = 0.18f;
    constexpr float mastTop = 58.0f;
    constexpr int radialCableCount = 32;
    constexpr int crossCableCount = 5;
    constexpr int stairCount = 12;
    constexpr float entranceHeight = 2.23f;
    constexpr float stairHalfWidth = 13.0f;
    constexpr float stairFrontZ = 46.0f;
    constexpr float stairTread = 1.2f;
    constexpr float apronInnerOffset = 0.2f;
    constexpr float apronOuterOffset = 1.5f;
    constexpr float apronInnerHeight = 4.13f;
    constexpr float apronOuterHeight = 3.55f;

    float roofApronHeight(float offset)
    {
        float fraction = (offset - apronInnerOffset) /
            (apronOuterOffset - apronInnerOffset);
        return apronInnerHeight +
            (apronOuterHeight - apronInnerHeight) * fraction;
    }

    struct CanopySample
    {
        Vec3 position;
        Vec3 normal;
    };

    CanopySample sampleCanopy(float fraction, float angle)
    {
        float towardTop = 1.0f - fraction;
        float profile = std::pow(towardTop, profilePower);
        float profileSlope = profilePower *
            std::pow(towardTop, profilePower - 1.0f);
        float radiusX = upperRingRadius +
            (baseRadiusX - upperRingRadius) * fraction;
        float radiusZ = upperRingRadius +
            (baseRadiusZ - upperRingRadius) * fraction;
        float height = canopyBottom + (canopyPeak - canopyBottom) * profile;
        float heightSlope = -(canopyPeak - canopyBottom) * profileSlope;
        float centerX = upperOffsetX * profile;
        float centerSlopeX = -upperOffsetX * profileSlope;
        float c = std::cos(angle);
        float s = std::sin(angle);

        Vec3 position{centerX + radiusX * c, height, radiusZ * s};
        Vec3 radial{
            centerSlopeX + (baseRadiusX - upperRingRadius) * c,
            heightSlope,
            (baseRadiusZ - upperRingRadius) * s
        };
        Vec3 around{-radiusX * s, 0, radiusZ * c};
        return {position, normalize(cross(around, radial))};
    }

    GLuint appendVertex(
        std::vector<float>& vertices,
        Vec3 position,
        Vec3 normal,
        float u = 0.0f,
        float v = 0.0f
    )
    {
        GLuint index = static_cast<GLuint>(vertices.size() / 8);
        vertices.insert(vertices.end(), {
            position.x, position.y, position.z,
            normal.x, normal.y, normal.z,
            u, v
        });
        return index;
    }

    void appendQuad(
        std::vector<float>& vertices,
        std::vector<GLuint>& indices,
        Vec3 a,
        Vec3 b,
        Vec3 c,
        Vec3 d
    )
    {
        Vec3 normal = normalize(cross(b - a, c - a));
        GLuint first = static_cast<GLuint>(vertices.size() / 8);
        appendVertex(vertices, a, normal);
        appendVertex(vertices, b, normal);
        appendVertex(vertices, c, normal);
        appendVertex(vertices, d, normal);
        indices.insert(indices.end(), {
            first, first + 1, first + 2,
            first, first + 2, first + 3
        });
    }

    void appendBox(
        std::vector<float>& vertices,
        std::vector<GLuint>& indices,
        Vec3 lower,
        Vec3 upper
    )
    {
        float x0 = lower.x, y0 = lower.y, z0 = lower.z;
        float x1 = upper.x, y1 = upper.y, z1 = upper.z;

        appendQuad(vertices, indices,
            {x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1});
        appendQuad(vertices, indices,
            {x1, y0, z0}, {x0, y0, z0}, {x0, y1, z0}, {x1, y1, z0});
        appendQuad(vertices, indices,
            {x0, y0, z0}, {x0, y0, z1}, {x0, y1, z1}, {x0, y1, z0});
        appendQuad(vertices, indices,
            {x1, y0, z1}, {x1, y0, z0}, {x1, y1, z0}, {x1, y1, z1});
        appendQuad(vertices, indices,
            {x0, y1, z1}, {x1, y1, z1}, {x1, y1, z0}, {x0, y1, z0});
        appendQuad(vertices, indices,
            {x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1});
    }

    void appendDisk(
        std::vector<float>& vertices,
        std::vector<GLuint>& indices,
        float radiusX,
        float radiusZ,
        float height,
        Vec3 offset = {0, 0, 0}
    )
    {
        GLuint center = appendVertex(
            vertices, {offset.x, height, offset.z}, {0, 1, 0}
        );
        GLuint first = static_cast<GLuint>(vertices.size() / 8);

        for (int segment = 0; segment <= radialSegments; ++segment)
        {
            float angle = 2.0f * pi * segment / radialSegments;
            appendVertex(vertices, {
                offset.x + radiusX * std::cos(angle), height,
                offset.z + radiusZ * std::sin(angle)
            }, {0, 1, 0});
        }

        for (int segment = 0; segment < radialSegments; ++segment)
        {
            indices.insert(indices.end(), {
                center, first + segment + 1, first + segment
            });
        }
    }

    void appendEllipticalWall(
        std::vector<float>& vertices,
        std::vector<GLuint>& indices,
        float radiusX,
        float radiusZ,
        float bottom,
        float top,
        Vec3 offset = {0, 0, 0},
        bool leaveEntranceOpening = false
    )
    {
        GLuint first = static_cast<GLuint>(vertices.size() / 8);

        for (int segment = 0; segment <= radialSegments; ++segment)
        {
            float angle = 2.0f * pi * segment / radialSegments;
            float x = offset.x + radiusX * std::cos(angle);
            float z = offset.z + radiusZ * std::sin(angle);
            Vec3 normal = normalize({
                std::cos(angle) / radiusX, 0, std::sin(angle) / radiusZ
            });
            float u = static_cast<float>(segment) / radialSegments;
            appendVertex(vertices, {x, bottom, z}, normal, u, 0);
            appendVertex(vertices, {x, top, z}, normal, u, 1);
        }

        for (int segment = 0; segment < radialSegments; ++segment)
        {
            // Eight front-facing sectors form a 28-meter-wide entrance opening.
            if (leaveEntranceOpening && segment >= 12 && segment < 20)
            {
                continue;
            }

            GLuint lower = first + 2 * segment;
            GLuint upper = lower + 1;
            GLuint nextLower = lower + 2;
            GLuint nextUpper = lower + 3;
            indices.insert(indices.end(), {
                lower, upper, nextLower,
                upper, nextUpper, nextLower
            });
        }
    }

    void appendTaperedMast(std::vector<float>& vertices, std::vector<GLuint>& indices)
    {
        float bottom = canopyPeak + 0.2f;
        float slope = (mastBaseRadius - mastTipRadius) / (mastTop - bottom);
        GLuint first = static_cast<GLuint>(vertices.size() / 8);

        for (int segment = 0; segment <= radialSegments; ++segment)
        {
            float angle = 2.0f * pi * segment / radialSegments;
            float c = std::cos(angle);
            float s = std::sin(angle);
            Vec3 normal = normalize({c, slope, s});
            appendVertex(vertices,
                {upperOffsetX + mastBaseRadius * c, bottom, mastBaseRadius * s},
                normal);
            appendVertex(vertices,
                {upperOffsetX + mastTipRadius * c, mastTop, mastTipRadius * s},
                normal);
        }

        for (int segment = 0; segment < radialSegments; ++segment)
        {
            GLuint lower = first + 2 * segment;
            GLuint upper = lower + 1;
            GLuint nextLower = lower + 2;
            GLuint nextUpper = lower + 3;
            indices.insert(indices.end(), {
                lower, upper, nextLower,
                upper, nextUpper, nextLower
            });
        }

        appendDisk(vertices, indices,
            mastTipRadius, mastTipRadius, mastTop, {upperOffsetX, 0, 0});
    }

    void appendCanopy(std::vector<float>& vertices, std::vector<GLuint>& indices)
    {
        GLuint first = static_cast<GLuint>(vertices.size() / 8);

        for (int ring = 0; ring <= heightSegments; ++ring)
        {
            float fraction = static_cast<float>(ring) / heightSegments;
            for (int segment = 0; segment <= radialSegments; ++segment)
            {
                float angle = 2.0f * pi * segment / radialSegments;
                CanopySample sample = sampleCanopy(fraction, angle);

                appendVertex(
                    vertices, sample.position, sample.normal,
                    static_cast<float>(segment) / radialSegments, fraction
                );
            }
        }

        constexpr GLuint stride = radialSegments + 1;

        for (int ring = 0; ring < heightSegments; ++ring)
        {
            for (int segment = 0; segment < radialSegments; ++segment)
            {
                GLuint upper = first + ring * stride + segment;
                GLuint lower = upper + stride;
                indices.insert(indices.end(), {
                    upper, upper + 1, lower,
                    lower, upper + 1, lower + 1
                });
            }
        }
    }

    void appendCableTube(
        std::vector<float>& vertices,
        std::vector<GLuint>& indices,
        const std::vector<CanopySample>& path,
        float radius,
        bool closed,
        float surfaceOffset = 0.18f
    )
    {
        constexpr int sides = 6;
        const int pointCount = static_cast<int>(path.size());
        GLuint first = static_cast<GLuint>(vertices.size() / 8);

        for (int point = 0; point < pointCount; ++point)
        {
            int previous = point > 0 ? point - 1 : (closed ? pointCount - 1 : 0);
            int next = point + 1 < pointCount ? point + 1 : (closed ? 0 : point);
            Vec3 tangent = normalize(path[next].position - path[previous].position);
            Vec3 surfaceNormal = path[point].normal;
            Vec3 side = normalize(cross(surfaceNormal, tangent));
            Vec3 center = path[point].position + surfaceNormal * surfaceOffset;

            for (int face = 0; face < sides; ++face)
            {
                float angle = 2.0f * pi * face / sides;
                Vec3 normal = surfaceNormal * std::cos(angle) +
                    side * std::sin(angle);
                appendVertex(vertices, center + normal * radius, normal);
            }
        }

        int segmentCount = closed ? pointCount : pointCount - 1;

        for (int point = 0; point < segmentCount; ++point)
        {
            int next = (point + 1) % pointCount;

            for (int face = 0; face < sides; ++face)
            {
                int nextFace = (face + 1) % sides;
                GLuint a = first + point * sides + face;
                GLuint b = first + point * sides + nextFace;
                GLuint c = first + next * sides + face;
                GLuint d = first + next * sides + nextFace;
                indices.insert(indices.end(), {a, c, b, b, c, d});
            }
        }
    }

    void appendCableNet(std::vector<float>& vertices, std::vector<GLuint>& indices)
    {
        std::vector<CanopySample> path;

        for (int cable = 0; cable < radialCableCount; ++cable)
        {
            path.clear();
            float angle = 2.0f * pi * cable / radialCableCount;

            for (int ring = 0; ring <= heightSegments; ++ring)
            {
                float fraction = static_cast<float>(ring) / heightSegments;
                path.push_back(sampleCanopy(fraction, angle));
            }

            appendCableTube(vertices, indices, path, 0.12f, false);
        }

        for (int cable = 1; cable <= crossCableCount; ++cable)
        {
            path.clear();
            float fraction = static_cast<float>(cable) / (crossCableCount + 1);

            for (int segment = 0; segment < radialSegments; ++segment)
            {
                float angle = 2.0f * pi * segment / radialSegments;
                path.push_back(sampleCanopy(fraction, angle));
            }

            appendCableTube(vertices, indices, path, 0.10f, true);
        }
    }

    void appendUpperBraces(std::vector<float>& vertices, std::vector<GLuint>& indices)
    {
        constexpr int braceCount = 8;

        for (int brace = 0; brace < braceCount; ++brace)
        {
            float angle = 2.0f * pi * brace / braceCount;
            float c = std::cos(angle);
            float s = std::sin(angle);
            Vec3 normal{c, 0, s};
            std::vector<CanopySample> path{
                {{upperOffsetX + 1.9f * c, canopyPeak + 0.18f, 1.9f * s}, normal},
                {{upperOffsetX + 0.30f * c, canopyPeak + 1.55f, 0.30f * s}, normal}
            };
            appendCableTube(vertices, indices, path, 0.075f, false, 0.0f);
        }
    }

    void appendApproach(std::vector<float>& vertices, std::vector<GLuint>& indices)
    {
        // The boulevard is 12 meters wide at local z = 80; the approach
        // broadens toward the 26-meter staircase at local z = 46.
        GLuint first = appendVertex(vertices, {-6.0f, 0.07f, 80.0f},
                                    {0, 1, 0}, 0.0f, 0.5f);
        appendVertex(vertices, {6.0f, 0.07f, 80.0f},
                     {0, 1, 0}, 12.0f / 8.0f, 0.5f);
        appendVertex(vertices, {stairHalfWidth, 0.07f, stairFrontZ},
                     {0, 1, 0}, (stairHalfWidth + 6.0f) / 8.0f, 4.75f);
        appendVertex(vertices, {-stairHalfWidth, 0.07f, stairFrontZ},
                     {0, 1, 0}, (6.0f - stairHalfWidth) / 8.0f, 4.75f);
        indices.insert(indices.end(), {
            first, first + 1, first + 2,
            first, first + 2, first + 3
        });
    }

    void appendStaircase(std::vector<float>& vertices, std::vector<GLuint>& indices)
    {
        constexpr float groundLevel = 0.07f;
        float rise = (entranceHeight - groundLevel) / stairCount;

        for (int step = 0; step < stairCount; ++step)
        {
            float front = stairFrontZ - step * stairTread;
            float back = front - stairTread;
            float top = groundLevel + (step + 1) * rise;
            float previousTop = groundLevel + step * rise;

            appendQuad(vertices, indices,
                {-stairHalfWidth, top, front}, {stairHalfWidth, top, front},
                {stairHalfWidth, top, back}, {-stairHalfWidth, top, back});
            appendQuad(vertices, indices,
                {-stairHalfWidth, previousTop, front},
                {stairHalfWidth, previousTop, front},
                {stairHalfWidth, top, front}, {-stairHalfWidth, top, front});
            appendQuad(vertices, indices,
                {-stairHalfWidth, groundLevel, back},
                {-stairHalfWidth, groundLevel, front},
                {-stairHalfWidth, top, front}, {-stairHalfWidth, top, back});
            appendQuad(vertices, indices,
                {stairHalfWidth, groundLevel, front},
                {stairHalfWidth, groundLevel, back},
                {stairHalfWidth, top, back}, {stairHalfWidth, top, front});
        }

        // The top step meets this landing at its back edge.
        appendBox(vertices, indices,
            {-stairHalfWidth, groundLevel, 23.8f},
            {stairHalfWidth, entranceHeight,
             stairFrontZ - stairCount * stairTread});
    }

    Vec3 lawnPoint(float side, float across, float towardFront)
    {
        // Taper the raised bank into an arc just inside the pedestrian ring.
        constexpr float innerX = 13.02f;
        constexpr float outerX = 32.0f;
        constexpr float walkingRingEdge = 51.9f;
        float frontWidth = outerX - 4.0f * towardFront * towardFront;
        float absoluteX = innerX + (frontWidth - innerX) * across;
        float back = baseRadiusZ * std::sqrt(
            1.0f - (absoluteX / baseRadiusX) * (absoluteX / baseRadiusX)
        ) + 0.04f;
        float front = std::sqrt(walkingRingEdge * walkingRingEdge -
                                absoluteX * absoluteX);
        float z = back + (front - back) * towardFront;
        float height = 0.08f + 2.15f *
            std::pow(1.0f - across, 1.4f) *
            std::pow(1.0f - towardFront, 1.2f);
        return {side * absoluteX, height, z};
    }

    void appendLandscapedSide(
        std::vector<float>& vertices,
        std::vector<GLuint>& indices,
        float side
    )
    {
        constexpr int acrossSegments = 10;
        constexpr int lengthSegments = 16;
        constexpr float epsilon = 0.001f;
        GLuint first = static_cast<GLuint>(vertices.size() / 8);

        for (int length = 0; length <= lengthSegments; ++length)
        {
            float v = static_cast<float>(length) / lengthSegments;

            for (int across = 0; across <= acrossSegments; ++across)
            {
                float u = static_cast<float>(across) / acrossSegments;
                Vec3 position = lawnPoint(side, u, v);
                Vec3 acrossTangent = lawnPoint(
                    side, std::min(1.0f, u + epsilon), v
                ) - lawnPoint(side, std::max(0.0f, u - epsilon), v);
                Vec3 lengthTangent = lawnPoint(
                    side, u, std::min(1.0f, v + epsilon)
                ) - lawnPoint(side, u, std::max(0.0f, v - epsilon));
                Vec3 normal = side > 0 ?
                    normalize(cross(lengthTangent, acrossTangent)) :
                    normalize(cross(acrossTangent, lengthTangent));
                appendVertex(vertices, position, normal);
            }
        }

        constexpr GLuint stride = acrossSegments + 1;

        for (int length = 0; length < lengthSegments; ++length)
        {
            for (int across = 0; across < acrossSegments; ++across)
            {
                GLuint a = first + length * stride + across;
                GLuint b = a + 1;
                GLuint c = a + stride;
                GLuint d = c + 1;

                if (side > 0)
                {
                    indices.insert(indices.end(), {a, c, b, b, c, d});
                }
                else
                {
                    indices.insert(indices.end(), {a, b, c, b, d, c});
                }
            }
        }
    }

    void appendEntranceGlass(
        std::vector<float>& vertices,
        std::vector<GLuint>& indices,
        bool doors
    )
    {
        for (int bay = 0; bay < 8; ++bay)
        {
            bool centralDoor = bay >= 2 && bay <= 5;

            if (centralDoor != doors)
            {
                continue;
            }

            float left = -10.0f + bay * 2.5f + 0.07f;
            float right = left + 2.36f;
            appendBox(vertices, indices,
                {left, entranceHeight + 0.06f, 23.59f},
                {right, centralDoor ? 5.03f : 5.56f, 23.82f});

            if (centralDoor)
            {
                appendBox(vertices, indices,
                    {left, 5.10f, 23.59f}, {right, 5.56f, 23.82f});
            }
        }
    }

    void appendEntranceFrames(std::vector<float>& vertices, std::vector<GLuint>& indices)
    {
        for (int post = 0; post <= 8; ++post)
        {
            float x = -10.0f + post * 2.5f;
            appendBox(vertices, indices,
                {x - 0.06f, entranceHeight, 23.82f},
                {x + 0.06f, 5.68f, 24.01f});
        }

        appendBox(vertices, indices,
            {-10.06f, entranceHeight, 23.82f},
            {10.06f, entranceHeight + 0.12f, 24.04f});
        appendBox(vertices, indices,
            {-10.06f, 5.56f, 23.82f}, {10.06f, 5.68f, 24.04f});
        appendBox(vertices, indices,
            {-5.0f, 5.03f, 23.82f}, {5.0f, 5.10f, 24.04f});

        // Side returns connect the recessed facade to the curved outer wall.
        appendQuad(vertices, indices,
            {10.0f, entranceHeight, 23.8f},
            {14.16f, 0.0f, 25.87f},
            {14.16f, canopyBottom, 25.87f},
            {10.0f, 6.2f, 23.8f});
        appendQuad(vertices, indices,
            {-14.16f, 0.0f, 25.87f},
            {-10.0f, entranceHeight, 23.8f},
            {-10.0f, 6.2f, 23.8f},
            {-14.16f, canopyBottom, 25.87f});
    }

    void appendDoorHandles(std::vector<float>& vertices, std::vector<GLuint>& indices)
    {
        for (float x : {-2.72f, -0.22f, 0.22f, 2.72f})
        {
            appendBox(vertices, indices,
                {x - 0.055f, 3.05f, 24.04f},
                {x + 0.055f, 3.70f, 24.14f});
        }
    }

    void appendEllipticalRim(
        std::vector<float>& vertices,
        std::vector<GLuint>& indices,
        float innerX,
        float innerZ,
        float outerX,
        float outerZ,
        float bottom,
        float top,
        Vec3 offset = {0, 0, 0},
        bool leaveEntranceOpening = false
    )
    {
        GLuint first = static_cast<GLuint>(vertices.size() / 8);

        for (int segment = 0; segment <= radialSegments; ++segment)
        {
            float angle = 2.0f * pi * segment / radialSegments;
            float c = std::cos(angle);
            float s = std::sin(angle);
            appendVertex(vertices, {
                offset.x + innerX * c, top, offset.z + innerZ * s
            }, {0, 1, 0});
            appendVertex(vertices, {
                offset.x + outerX * c, top, offset.z + outerZ * s
            }, {0, 1, 0});
        }

        for (int segment = 0; segment < radialSegments; ++segment)
        {
            if (leaveEntranceOpening && segment >= 12 && segment < 20)
            {
                continue;
            }

            GLuint inner = first + 2 * segment;
            GLuint outer = inner + 1;
            GLuint nextInner = inner + 2;
            GLuint nextOuter = inner + 3;
            indices.insert(indices.end(), {
                inner, nextInner, outer,
                outer, nextInner, nextOuter
            });
        }

        appendEllipticalWall(
            vertices, indices, outerX, outerZ,
            bottom, top, offset, leaveEntranceOpening
        );
    }

    void appendPerimeterPanelSeams(std::vector<float>& vertices,
                                   std::vector<GLuint>& indices)
    {
        constexpr float halfAngle = 0.0025f;
        constexpr float seamRadiusX = baseRadiusX + 0.07f;
        constexpr float seamRadiusZ = baseRadiusZ + 0.07f;

        for (int segment = 0; segment < radialSegments; segment += 2)
        {
            // Keep the front glass entrance free of wall panel joints.
            if (segment >= 12 && segment <= 20)
            {
                continue;
            }

            float angle = 2.0f * pi * segment / radialSegments;
            float left = angle - halfAngle;
            float right = angle + halfAngle;
            appendQuad(vertices, indices,
                {seamRadiusX * std::cos(right), 1.08f,
                 seamRadiusZ * std::sin(right)},
                {seamRadiusX * std::cos(left), 1.08f,
                 seamRadiusZ * std::sin(left)},
                {seamRadiusX * std::cos(left), 3.52f,
                 seamRadiusZ * std::sin(left)},
                {seamRadiusX * std::cos(right), 3.52f,
                 seamRadiusZ * std::sin(right)});
        }
    }

    void appendRoofTransition(std::vector<float>& vertices,
                              std::vector<GLuint>& indices)
    {
        for (int segment = 0; segment < radialSegments; ++segment)
        {
            // The sloping roof apron stops at the existing entrance opening.
            if (segment >= 12 && segment < 20)
            {
                continue;
            }

            float start = 2.0f * pi * segment / radialSegments;
            float end = 2.0f * pi * (segment + 1) / radialSegments;
            appendQuad(vertices, indices,
                {(baseRadiusX + apronInnerOffset) * std::cos(start),
                 apronInnerHeight,
                 (baseRadiusZ + apronInnerOffset) * std::sin(start)},
                {(baseRadiusX + apronInnerOffset) * std::cos(end),
                 apronInnerHeight,
                 (baseRadiusZ + apronInnerOffset) * std::sin(end)},
                {(baseRadiusX + apronOuterOffset) * std::cos(end),
                 apronOuterHeight,
                 (baseRadiusZ + apronOuterOffset) * std::sin(end)},
                {(baseRadiusX + apronOuterOffset) * std::cos(start),
                 apronOuterHeight,
                 (baseRadiusZ + apronOuterOffset) * std::sin(start)});
        }

        // A slim outer fascia gives the apron a finished edge.
        appendEllipticalWall(vertices, indices,
                             baseRadiusX + apronOuterOffset,
                             baseRadiusZ + apronOuterOffset,
                             3.43f, apronOuterHeight, {0, 0, 0}, true);
    }

    Vec3 perimeterVentPoint(float angle, float offset, float lift)
    {
        return {(baseRadiusX + offset) * std::cos(angle),
                roofApronHeight(offset) + lift,
                (baseRadiusZ + offset) * std::sin(angle)};
    }

    void appendPerimeterVent(std::vector<float>& vertices,
                             std::vector<GLuint>& indices, float centerAngle,
                             float bottomLift, float topLift)
    {
        constexpr int arcSegments = 4;
        constexpr float halfAngle = 0.03f;
        constexpr float innerOffset = 0.55f;
        constexpr float outerOffset = 1.25f;
        float start = centerAngle - halfAngle;
        float end = centerAngle + halfAngle;

        for (int segment = 0; segment < arcSegments; ++segment)
        {
            float a = start + (end - start) * segment / arcSegments;
            float b = start + (end - start) * (segment + 1) / arcSegments;

            appendQuad(vertices, indices,
                perimeterVentPoint(a, innerOffset, topLift),
                perimeterVentPoint(b, innerOffset, topLift),
                perimeterVentPoint(b, outerOffset, topLift),
                perimeterVentPoint(a, outerOffset, topLift));
            appendQuad(vertices, indices,
                perimeterVentPoint(a, innerOffset, bottomLift),
                perimeterVentPoint(b, innerOffset, bottomLift),
                perimeterVentPoint(b, innerOffset, topLift),
                perimeterVentPoint(a, innerOffset, topLift));
            appendQuad(vertices, indices,
                perimeterVentPoint(b, outerOffset, bottomLift),
                perimeterVentPoint(a, outerOffset, bottomLift),
                perimeterVentPoint(a, outerOffset, topLift),
                perimeterVentPoint(b, outerOffset, topLift));
        }

        appendQuad(vertices, indices,
            perimeterVentPoint(start, outerOffset, bottomLift),
            perimeterVentPoint(start, innerOffset, bottomLift),
            perimeterVentPoint(start, innerOffset, topLift),
            perimeterVentPoint(start, outerOffset, topLift));
        appendQuad(vertices, indices,
            perimeterVentPoint(end, innerOffset, bottomLift),
            perimeterVentPoint(end, outerOffset, bottomLift),
            perimeterVentPoint(end, outerOffset, topLift),
            perimeterVentPoint(end, innerOffset, topLift));
    }
}

KhanShatyr::KhanShatyr(Vec3 position)
    : model(modelMatrix(position, 0, {1, 1, 1}))
{
    std::vector<float> vertices;
    std::vector<GLuint> indices;

    auto recordRange = [&indices](IndexRange& range, std::size_t start)
    {
        range.firstIndex = start;
        range.count = static_cast<GLsizei>(indices.size() - start);
    };

    std::size_t start = indices.size();
    appendDisk(vertices, indices, baseRadiusX, baseRadiusZ, 0.06f);
    recordRange(floorRange, start);

    start = indices.size();
    appendEllipticalWall(
        vertices, indices, baseRadiusX, baseRadiusZ,
        0, 1.05f, {0, 0, 0}, true
    );
    recordRange(plinthRange, start);

    start = indices.size();
    appendEllipticalWall(
        vertices, indices, baseRadiusX, baseRadiusZ,
        1.05f, 3.55f, {0, 0, 0}, true
    );
    recordRange(wallRange, start);

    start = indices.size();
    appendEllipticalWall(
        vertices, indices, baseRadiusX, baseRadiusZ,
        3.55f, canopyBottom, {0, 0, 0}, true
    );
    recordRange(upperWallRange, start);

    start = indices.size();
    appendPerimeterPanelSeams(vertices, indices);
    recordRange(panelSeamRange, start);

    start = indices.size();
    appendRoofTransition(vertices, indices);
    recordRange(roofTransitionRange, start);

    constexpr std::array<float, 4> serviceAngles{{210.0f, 250.0f,
                                                    290.0f, 330.0f}};
    start = indices.size();
    for (float degrees : serviceAngles)
    {
        appendPerimeterVent(vertices, indices, degrees * pi / 180.0f,
                            -0.06f, 0.20f);
    }
    recordRange(ventHousingRange, start);

    start = indices.size();
    for (float degrees : serviceAngles)
    {
        appendPerimeterVent(vertices, indices, degrees * pi / 180.0f,
                            0.20f, 0.25f);
    }
    recordRange(ventCapRange, start);

    start = indices.size();
    appendCanopy(vertices, indices);
    recordRange(canopyRange, start);

    start = indices.size();
    appendCableNet(vertices, indices);
    recordRange(cableRange, start);

    start = indices.size();
    appendEllipticalRim(
        vertices, indices,
        baseRadiusX - 0.7f, baseRadiusZ - 0.7f,
        baseRadiusX + 0.4f, baseRadiusZ + 0.4f,
        3.85f, 4.27f, {0, 0, 0}, true
    );
    appendEllipticalRim(
        vertices, indices,
        baseRadiusX - 0.4f, baseRadiusZ - 0.4f,
        baseRadiusX + 1.0f, baseRadiusZ + 1.0f,
        0.0f, 0.55f
    );
    recordRange(rimRange, start);

    start = indices.size();
    Vec3 upperCenter{upperOffsetX, 0, 0};
    appendEllipticalRim(
        vertices, indices, 1.9f, 1.9f, 3.15f, 3.15f,
        canopyPeak - 1.5f, canopyPeak + 0.45f, upperCenter
    );
    appendDisk(vertices, indices, 1.9f, 1.9f, canopyPeak + 0.25f, upperCenter);
    recordRange(serviceRingRange, start);

    start = indices.size();
    appendEllipticalRim(
        vertices, indices, 3.02f, 3.02f, 3.25f, 3.25f,
        canopyPeak - 1.15f, canopyPeak - 1.0f, upperCenter
    );
    appendEllipticalRim(
        vertices, indices, 3.02f, 3.02f, 3.25f, 3.25f,
        canopyPeak + 0.28f, canopyPeak + 0.5f, upperCenter
    );
    recordRange(serviceTrimRange, start);

    start = indices.size();
    appendTaperedMast(vertices, indices);
    appendUpperBraces(vertices, indices);
    recordRange(mastRange, start);

    start = indices.size();
    appendApproach(vertices, indices);
    recordRange(approachRange, start);

    start = indices.size();
    appendStaircase(vertices, indices);
    recordRange(stairRange, start);

    start = indices.size();
    appendLandscapedSide(vertices, indices, -1.0f);
    appendLandscapedSide(vertices, indices, 1.0f);
    recordRange(lawnRange, start);

    start = indices.size();
    appendEntranceGlass(vertices, indices, false);
    recordRange(storefrontGlassRange, start);

    start = indices.size();
    appendEntranceGlass(vertices, indices, true);
    recordRange(doorGlassRange, start);

    start = indices.size();
    appendEntranceFrames(vertices, indices);
    recordRange(frameRange, start);

    start = indices.size();
    appendDoorHandles(vertices, indices);
    recordRange(handleRange, start);

    start = indices.size();
    appendBox(vertices, indices,
        {-10.1f, 5.70f, 23.79f}, {10.1f, 6.20f, 24.00f});
    recordRange(fasciaRange, start);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
        vertices.data(), GL_STATIC_DRAW
    );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(GLuint)),
        indices.data(), GL_STATIC_DRAW
    );

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
        reinterpret_cast<const void*>(3 * sizeof(float))
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
        reinterpret_cast<const void*>(6 * sizeof(float))
    );
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

KhanShatyr::~KhanShatyr()
{
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void KhanShatyr::drawRange(
    const IndexRange& range,
    Vec3 color,
    GLint objectColorLocation
) const
{
    glUniform3f(objectColorLocation, color.x, color.y, color.z);
    glDrawElements(
        GL_TRIANGLES, range.count, GL_UNSIGNED_INT,
        reinterpret_cast<const void*>(range.firstIndex * sizeof(GLuint))
    );
}

void KhanShatyr::render(
    GLint modelLocation,
    GLint objectColorLocation,
    GLint khanCanopyLocation,
    GLint khanCableLocation,
    GLint shaftGlassLocation,
    GLint useGrassLocation,
    GLint useTextureLocation
) const
{
    glBindVertexArray(vao);
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model.data());
    glUniform1i(khanCanopyLocation, GL_FALSE);
    glUniform1i(khanCableLocation, GL_FALSE);
    drawRange(floorRange, {0.63f, 0.67f, 0.68f}, objectColorLocation);
    drawRange(plinthRange, {0.63f, 0.68f, 0.70f}, objectColorLocation);
    drawRange(wallRange, {0.74f, 0.78f, 0.79f}, objectColorLocation);
    drawRange(upperWallRange, {0.82f, 0.85f, 0.85f}, objectColorLocation);
    drawRange(panelSeamRange, {0.64f, 0.69f, 0.71f}, objectColorLocation);
    drawRange(roofTransitionRange, {0.86f, 0.88f, 0.88f}, objectColorLocation);
    drawRange(ventHousingRange, {0.78f, 0.81f, 0.82f}, objectColorLocation);
    drawRange(ventCapRange, {0.85f, 0.87f, 0.87f}, objectColorLocation);

    glUniform1i(khanCanopyLocation, GL_TRUE);
    drawRange(canopyRange, {0.97f, 0.98f, 0.98f}, objectColorLocation);
    glUniform1i(khanCanopyLocation, GL_FALSE);

    glUniform1i(khanCableLocation, GL_TRUE);
    drawRange(cableRange, {0.97f, 0.98f, 0.98f}, objectColorLocation);
    glUniform1i(khanCableLocation, GL_FALSE);

    drawRange(rimRange, {0.76f, 0.81f, 0.83f}, objectColorLocation);
    drawRange(serviceRingRange, {0.77f, 0.81f, 0.83f}, objectColorLocation);
    drawRange(serviceTrimRange, {0.90f, 0.92f, 0.93f}, objectColorLocation);
    drawRange(mastRange, {0.89f, 0.91f, 0.92f}, objectColorLocation);

    glUniform1i(useTextureLocation, GL_TRUE);
    drawRange(approachRange, {0.52f, 0.53f, 0.54f}, objectColorLocation);
    glUniform1i(useTextureLocation, GL_FALSE);
    drawRange(stairRange, {0.81f, 0.82f, 0.80f}, objectColorLocation);

    glUniform1i(useGrassLocation, GL_TRUE);
    drawRange(lawnRange, {0.35f, 0.49f, 0.32f}, objectColorLocation);
    glUniform1i(useGrassLocation, GL_FALSE);

    glUniform1i(shaftGlassLocation, GL_TRUE);
    drawRange(storefrontGlassRange, {0.19f, 0.30f, 0.37f}, objectColorLocation);
    drawRange(doorGlassRange, {0.26f, 0.37f, 0.43f}, objectColorLocation);
    glUniform1i(shaftGlassLocation, GL_FALSE);

    drawRange(frameRange, {0.74f, 0.78f, 0.79f}, objectColorLocation);
    drawRange(handleRange, {0.86f, 0.88f, 0.87f}, objectColorLocation);
    drawRange(fasciaRange, {0.78f, 0.81f, 0.81f}, objectColorLocation);
    glBindVertexArray(0);
}

void KhanShatyr::renderShadow(SunShadows& shadows)const
{
    if(!shadows.intersects({model[12],model[13]+25,model[14]},{40,28,32}))return;
    shadows.instanced(false);glUniformMatrix4fv(shadows.modelLocation(),1,GL_FALSE,model.data());glBindVertexArray(vao);
    // The existing membrane renders opaque. Skip detailed cables, facade/door glass and site paving.
    for(const auto* r:{&plinthRange,&wallRange,&upperWallRange,&roofTransitionRange,&canopyRange,&rimRange,&mastRange})if(r->count){
        glDrawElements(GL_TRIANGLES,r->count,GL_UNSIGNED_INT,reinterpret_cast<void*>(r->firstIndex*sizeof(GLuint)));shadows.record(r->count);}
    shadows.countCasters(1);
}
