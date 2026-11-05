#include "Mesh.h"
#include "Math3D.h"

#include <array>
#include <cmath>
#include <vector>

namespace
{
    Vec3 spherePoint(int ring, int segment, int ringCount, int segmentCount)
    {
        if (ring == 0)
        {
            return {0, 1, 0};
        }

        if (ring == ringCount)
        {
            return {0, -1, 0};
        }

        float latitude = pi * static_cast<float>(ring) / ringCount;
        float longitude = 2.0f * pi * static_cast<float>(segment % segmentCount) / segmentCount;
        float radius = std::sin(latitude);

        return {
            radius * std::cos(longitude),
            std::cos(latitude),
            radius * std::sin(longitude)
        };
    }

    constexpr int circleSegments = 96;
    constexpr int gardenSegments = 32;
    constexpr int bedSegments = 20;

    void appendAnnulus(
        std::vector<float>& vertices,
        float innerRadius,
        float outerRadius,
        float startAngle = 0.0f,
        float endAngle = 2.0f * pi,
        int segments = circleSegments,
        float height = 0.0f
    )
    {
        auto appendVertex = [&vertices, height](float radius, float angle)
        {
            float x = radius * std::cos(angle);
            float z = radius * std::sin(angle);

            // One UV unit spans the 48-meter diameter; the scene repeats it four times.
            vertices.insert(vertices.end(), {
                x, height, z, 0, 1, 0,
                x / 48.0f + 0.5f, z / 48.0f + 0.5f
            });
        };

        for (int segment = 0; segment < segments; ++segment)
        {
            float angle = startAngle + (endAngle - startAngle) * segment / segments;
            float nextAngle = startAngle + (endAngle - startAngle) * (segment + 1) / segments;

            appendVertex(innerRadius, angle);
            appendVertex(innerRadius, nextAngle);
            appendVertex(outerRadius, nextAngle);
            appendVertex(innerRadius, angle);
            appendVertex(outerRadius, nextAngle);
            appendVertex(outerRadius, angle);
        }
    }

    Vec3 bedPoint(float radius, float angle)
    {
        return {radius * std::cos(angle), 0, radius * std::sin(angle)};
    }

    void appendBedWall(std::vector<float>& vertices, Vec3 start, Vec3 end, Vec3 normal)
    {
        Vec3 upperStart = start + Vec3{0, 0.09f, 0};
        Vec3 upperEnd = end + Vec3{0, 0.09f, 0};

        auto appendVertex = [&vertices, normal](Vec3 position)
        {
            vertices.insert(vertices.end(), {
                position.x, position.y, position.z,
                normal.x, normal.y, normal.z, 0, 0
            });
        };

        appendVertex(start);
        appendVertex(end);
        appendVertex(upperEnd);
        appendVertex(start);
        appendVertex(upperEnd);
        appendVertex(upperStart);
    }
}

Mesh::Mesh()
{
    const float vertices[] = {
        -50, 0, -50,
        -50, 0, 50,
        50, 0, 50,
        -50, 0, -50,
        50, 0, 50,
        50, 0, -50,

        // Unit cube centered at the origin, with two triangles per face.
        // Front (+Z).
        -0.5f, -0.5f, 0.5f,
         0.5f, -0.5f, 0.5f,
         0.5f,  0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f,
         0.5f,  0.5f, 0.5f,
        -0.5f,  0.5f, 0.5f,

        // Back (-Z).
         0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,

        // Left (-X).
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,

        // Right (+X).
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,

        // Top (+Y).
        -0.5f, 0.5f,  0.5f,
         0.5f, 0.5f,  0.5f,
         0.5f, 0.5f, -0.5f,
        -0.5f, 0.5f,  0.5f,
         0.5f, 0.5f, -0.5f,
        -0.5f, 0.5f, -0.5f,

        // Bottom (-Y).
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f
    };

    std::vector<float> meshVertices(vertices, vertices + sizeof(vertices) / sizeof(float));
    sphereFirstVertex = static_cast<GLint>(meshVertices.size() / 3);

    auto appendVertex = [&meshVertices](Vec3 vertex)
    {
        meshVertices.push_back(vertex.x);
        meshVertices.push_back(vertex.y);
        meshVertices.push_back(vertex.z);
    };

    // Latitude/longitude bands form a unit sphere; skip degenerate pole triangles.
    constexpr int ringCount = 24;
    constexpr int segmentCount = 32;

    for (int ring = 0; ring < ringCount; ++ring)
    {
        for (int segment = 0; segment < segmentCount; ++segment)
        {
            Vec3 topLeft = spherePoint(ring, segment, ringCount, segmentCount);
            Vec3 topRight = spherePoint(ring, segment + 1, ringCount, segmentCount);
            Vec3 bottomLeft = spherePoint(ring + 1, segment, ringCount, segmentCount);
            Vec3 bottomRight = spherePoint(ring + 1, segment + 1, ringCount, segmentCount);

            if (ring < ringCount - 1)
            {
                appendVertex(topLeft);
                appendVertex(bottomRight);
                appendVertex(bottomLeft);
            }

            if (ring > 0)
            {
                appendVertex(topLeft);
                appendVertex(topRight);
                appendVertex(bottomRight);
            }
        }
    }

    sphereVertexCount = static_cast<GLsizei>(meshVertices.size() / 3) - sphereFirstVertex;

    // Preserve the existing positions and draw ranges; add one normal per vertex.
    const Vec3 faceNormals[] = {
        {0, 1, 0},  // Ground.
        {0, 0, 1},  // Cube front.
        {0, 0, -1}, // Cube back.
        {-1, 0, 0}, // Cube left.
        {1, 0, 0},  // Cube right.
        {0, 1, 0},  // Cube top.
        {0, -1, 0}  // Cube bottom.
    };
    std::vector<float> litVertices;
    litVertices.reserve(meshVertices.size() / 3 * 8);

    for (std::size_t i = 0; i < meshVertices.size() / 3; ++i)
    {
        Vec3 position{
            meshVertices[i * 3],
            meshVertices[i * 3 + 1],
            meshVertices[i * 3 + 2]
        };
        Vec3 normal;

        if (i < static_cast<std::size_t>(sphereFirstVertex))
        {
            normal = faceNormals[i / 6];
        }
        else
        {
            // A unit sphere's outward normal follows its radius.
            normal = normalize(position);
        }

        litVertices.insert(litVertices.end(), {
            position.x, position.y, position.z,
            normal.x, normal.y, normal.z,
            position.x + 0.5f, position.z + 0.5f
        });
    }

    // A square-based pyramid tapers from y = -0.5 to a true point at y = 0.5.
    const Vec3 tipVertices[] = {
        {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, 0.5f},
        {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f},
        {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {0, 0.5f, 0},
        {0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {0, 0.5f, 0},
        {-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f}, {0, 0.5f, 0},
        {0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, -0.5f}, {0, 0.5f, 0}
    };
    tipFirstVertex = static_cast<GLint>(litVertices.size() / 8);

    for (int triangle = 0; triangle < 6; ++triangle)
    {
        const Vec3* face = tipVertices + triangle * 3;
        Vec3 normal = normalize(cross(face[1] - face[0], face[2] - face[0]));

        for (int vertex = 0; vertex < 3; ++vertex)
        {
            const Vec3& position = face[vertex];
            litVertices.insert(litVertices.end(), {
                position.x, position.y, position.z,
                normal.x, normal.y, normal.z, 0, 0
            });
        }
    }

    circularPlazaFirstVertex = static_cast<GLint>(litVertices.size() / 8);
    appendAnnulus(litVertices, 10.1f, 24.0f);

    circularBorderFirstVertex = static_cast<GLint>(litVertices.size() / 8);
    appendAnnulus(litVertices, 10.1f, 10.45f);
    appendAnnulus(litVertices, 23.65f, 24.0f);

    // A unit disk and narrow ring can be scaled into matching oval parterres.
    unitDiskFirstVertex = static_cast<GLint>(litVertices.size() / 8);
    for (int segment = 0; segment < circleSegments; ++segment)
    {
        float angle = 2.0f * pi * segment / circleSegments;
        float nextAngle = 2.0f * pi * (segment + 1) / circleSegments;

        for (Vec3 point : {Vec3{0, 0, 0},
                           Vec3{0.965f * std::cos(nextAngle), 0,
                                0.965f * std::sin(nextAngle)},
                           Vec3{0.965f * std::cos(angle), 0,
                                0.965f * std::sin(angle)}})
        {
            litVertices.insert(litVertices.end(), {
                point.x, point.y, point.z, 0, 1, 0,
                point.x * 0.5f + 0.5f, point.z * 0.5f + 0.5f
            });
        }
    }

    unitRingFirstVertex = static_cast<GLint>(litVertices.size() / 8);
    appendAnnulus(litVertices, 0.965f, 1.0f);

    // The side wall serves the shallow round fountain basins.
    unitCylinderWallFirstVertex = static_cast<GLint>(litVertices.size() / 8);
    auto appendCylinderPoint = [&litVertices](float angle, float y)
    {
        float x = std::cos(angle);
        float z = std::sin(angle);
        litVertices.insert(litVertices.end(), {x, y, z, x, 0, z, 0, 0});
    };

    for (int segment = 0; segment < circleSegments; ++segment)
    {
        float angle = 2.0f * pi * segment / circleSegments;
        float nextAngle = 2.0f * pi * (segment + 1) / circleSegments;
        appendCylinderPoint(angle, 0);
        appendCylinderPoint(nextAngle, 1);
        appendCylinderPoint(nextAngle, 0);
        appendCylinderPoint(angle, 0);
        appendCylinderPoint(angle, 1);
        appendCylinderPoint(nextAngle, 1);
    }

    // Five slender, curved jets share one draw call for each fountain.
    struct FountainJet
    {
        float x;
        float z;
        float height;
        float bendX;
        float bendZ;
        float radius;
    };
    constexpr std::array<FountainJet, 5> jets{{
        {0, 0, 3.0f, 0, 0, 0.11f},
        {-3.0f, 0, 2.1f, -0.55f, 0, 0.08f},
        {3.0f, 0, 2.1f, 0.55f, 0, 0.08f},
        {0, -4.5f, 2.1f, 0, -0.65f, 0.08f},
        {0, 4.5f, 2.1f, 0, 0.65f, 0.08f}
    }};
    constexpr int jetRings = 12;
    constexpr int jetSides = 8;
    fountainJetFirstVertex = static_cast<GLint>(litVertices.size() / 8);

    for (const FountainJet& jet : jets)
    {
        auto appendJetVertex = [&litVertices, &jet](float t, float angle)
        {
            float radialX = std::cos(angle);
            float radialZ = std::sin(angle);
            float radius = jet.radius * (1.0f - 0.75f * t);
            Vec3 position{
                jet.x + jet.bendX * t * t + radialX * radius,
                jet.height * t,
                jet.z + jet.bendZ * t * t + radialZ * radius
            };
            Vec3 normal = normalize(Vec3{radialX, 0.06f, radialZ});
            litVertices.insert(litVertices.end(), {
                position.x, position.y, position.z,
                normal.x, normal.y, normal.z, 0, 0
            });
        };

        for (int ring = 0; ring < jetRings; ++ring)
        {
            float lower = static_cast<float>(ring) / jetRings;
            float upper = static_cast<float>(ring + 1) / jetRings;

            for (int side = 0; side < jetSides; ++side)
            {
                float angle = 2.0f * pi * side / jetSides;
                float nextAngle = 2.0f * pi * (side + 1) / jetSides;
                appendJetVertex(lower, angle);
                appendJetVertex(upper, nextAngle);
                appendJetVertex(lower, nextAngle);
                appendJetVertex(lower, angle);
                appendJetVertex(upper, angle);
                appendJetVertex(upper, nextAngle);
            }
        }
    }
    fountainJetVertexCount = static_cast<GLsizei>(litVertices.size() / 8)
        - fountainJetFirstVertex;

    // A rounded rectangle forms one formal garden bed, reusable at any scale.
    unitCapsuleFirstVertex = static_cast<GLint>(litVertices.size() / 8);
    auto appendCapsuleVertex = [&litVertices](float x, float z)
    {
        litVertices.insert(litVertices.end(), {
            x, 0, z, 0, 1, 0, x * 0.5f + 0.5f, z / 6.0f + 0.5f
        });
    };
    auto appendCapsuleTriangle = [&appendCapsuleVertex](
        float ax, float az, float bx, float bz, float cx, float cz)
    {
        appendCapsuleVertex(ax, az);
        appendCapsuleVertex(bx, bz);
        appendCapsuleVertex(cx, cz);
    };

    appendCapsuleTriangle(-1, -2, 1, 2, 1, -2);
    appendCapsuleTriangle(-1, -2, -1, 2, 1, 2);

    constexpr int capSegments = 32;
    for (int segment = 0; segment < capSegments; ++segment)
    {
        float nearAngle = pi + pi * segment / capSegments;
        float nearNext = pi + pi * (segment + 1) / capSegments;
        appendCapsuleTriangle(
            0, -2, std::cos(nearNext), -2 + std::sin(nearNext),
            std::cos(nearAngle), -2 + std::sin(nearAngle)
        );

        float farAngle = pi * segment / capSegments;
        float farNext = pi * (segment + 1) / capSegments;
        appendCapsuleTriangle(
            0, 2, std::cos(farNext), 2 + std::sin(farNext),
            std::cos(farAngle), 2 + std::sin(farAngle)
        );
    }
    unitCapsuleVertexCount = static_cast<GLsizei>(litVertices.size() / 8) -
        unitCapsuleFirstVertex;

    // One tapered stone surface ties the approach, parterres and palace together.
    ceremonialCourtFirstVertex = static_cast<GLint>(litVertices.size() / 8);
    struct CourtSection
    {
        float z;
        float halfWidth;
    };
    constexpr std::array<CourtSection, 9> courtSections{{
        {-150, 42}, {-149, 48}, {-146, 55}, {-140, 59}, {-130, 62},
        {-110, 64}, {-90, 66}, {-70, 68}, {-34, 68}
    }};

    auto appendCourtVertex = [&litVertices](Vec3 point, Vec3 normal)
    {
        litVertices.insert(litVertices.end(), {
            point.x, point.y, point.z, normal.x, normal.y, normal.z,
            point.x / 12.0f + 0.5f, point.z / 12.0f
        });
    };

    for (std::size_t section = 0; section + 1 < courtSections.size(); ++section)
    {
        const CourtSection& a = courtSections[section];
        const CourtSection& b = courtSections[section + 1];
        Vec3 leftA{-a.halfWidth, 0, a.z};
        Vec3 rightA{a.halfWidth, 0, a.z};
        Vec3 leftB{-b.halfWidth, 0, b.z};
        Vec3 rightB{b.halfWidth, 0, b.z};

        appendCourtVertex(leftA, {0, 1, 0});
        appendCourtVertex(rightB, {0, 1, 0});
        appendCourtVertex(rightA, {0, 1, 0});
        appendCourtVertex(leftA, {0, 1, 0});
        appendCourtVertex(leftB, {0, 1, 0});
        appendCourtVertex(rightB, {0, 1, 0});

        // A seven-centimeter skirt joins the raised paving to the terrain.
        Vec3 lowerLeftA{leftA.x, -0.07f, a.z};
        Vec3 lowerLeftB{leftB.x, -0.07f, b.z};
        appendCourtVertex(lowerLeftA, {-1, 0, 0});
        appendCourtVertex(leftB, {-1, 0, 0});
        appendCourtVertex(leftA, {-1, 0, 0});
        appendCourtVertex(lowerLeftA, {-1, 0, 0});
        appendCourtVertex(lowerLeftB, {-1, 0, 0});
        appendCourtVertex(leftB, {-1, 0, 0});

        Vec3 lowerRightA{rightA.x, -0.07f, a.z};
        Vec3 lowerRightB{rightB.x, -0.07f, b.z};
        appendCourtVertex(lowerRightA, {1, 0, 0});
        appendCourtVertex(rightA, {1, 0, 0});
        appendCourtVertex(rightB, {1, 0, 0});
        appendCourtVertex(lowerRightA, {1, 0, 0});
        appendCourtVertex(rightB, {1, 0, 0});
        appendCourtVertex(lowerRightB, {1, 0, 0});
    }

    for (bool front : {true, false})
    {
        const CourtSection& edge = front ? courtSections.front() : courtSections.back();
        float normalZ = front ? -1.0f : 1.0f;
        Vec3 lowerLeft{-edge.halfWidth, -0.07f, edge.z};
        Vec3 lowerRight{edge.halfWidth, -0.07f, edge.z};
        Vec3 upperLeft{-edge.halfWidth, 0, edge.z};
        Vec3 upperRight{edge.halfWidth, 0, edge.z};
        appendCourtVertex(lowerLeft, {0, 0, normalZ});
        appendCourtVertex(lowerRight, {0, 0, normalZ});
        appendCourtVertex(upperRight, {0, 0, normalZ});
        appendCourtVertex(lowerLeft, {0, 0, normalZ});
        appendCourtVertex(upperRight, {0, 0, normalZ});
        appendCourtVertex(upperLeft, {0, 0, normalZ});
    }
    ceremonialCourtVertexCount = static_cast<GLsizei>(litVertices.size() / 8) -
        ceremonialCourtFirstVertex;

    // Flower positions are local to one 22 x 54 meter lawn. Both palace beds
    // reuse these batches, so the two parterres have the same mirrored design.
    std::array<std::vector<Vec3>, 4> parterreFlowers;
    auto addFlower = [&parterreFlowers](int color, float x, float z)
    {
        parterreFlowers[color].push_back({x, 0, z});
    };

    // Curving red and white borders frame the long edges and rounded ends.
    constexpr int borderSamples = 64;
    for (int sample = 0; sample <= borderSamples; ++sample)
    {
        float z = -20.0f + 40.0f * sample / borderSamples;
        float outerX = 7.2f + 0.6f * (z * z / 400.0f);

        for (float side : {-1.0f, 1.0f})
        {
            addFlower(0, side * outerX, z);
            addFlower(2, side * (outerX - 0.72f), z);
        }
    }

    constexpr int arcSamples = 24;
    for (float end : {-1.0f, 1.0f})
    {
        for (int sample = 1; sample < arcSamples; ++sample)
        {
            float angle = pi * sample / arcSamples;
            addFlower(0, 7.8f * std::cos(angle),
                      end * (18.0f + 5.0f * std::sin(angle)));
        }
    }

    // Three yellow diamond outlines and restrained purple centers repeat
    // down the garden axis while leaving broad green gaps between motifs.
    constexpr std::array<Vec3, 4> diamondCorners{{
        {0, 0, 5}, {3.6f, 0, 0}, {0, 0, -5}, {-3.6f, 0, 0}
    }};
    constexpr std::array<Vec3, 7> purpleCenters{{
        {0, 0, 0}, {-1.5f, 0, -1.2f}, {1.5f, 0, -1.2f},
        {-1.5f, 0, 1.2f}, {1.5f, 0, 1.2f},
        {0, 0, -2.3f}, {0, 0, 2.3f}
    }};

    for (float centerZ : {-13.0f, 0.0f, 13.0f})
    {
        for (std::size_t edge = 0; edge < diamondCorners.size(); ++edge)
        {
            const Vec3& a = diamondCorners[edge];
            const Vec3& b = diamondCorners[(edge + 1) % diamondCorners.size()];

            for (int sample = 0; sample < 8; ++sample)
            {
                float t = static_cast<float>(sample) / 8.0f;
                addFlower(1, a.x + (b.x - a.x) * t,
                          centerZ + a.z + (b.z - a.z) * t);
            }
        }

        for (const Vec3& center : purpleCenters)
        {
            addFlower(3, center.x, centerZ + center.z);
        }
    }

    // Crossed green quads form short stems in a single draw per parterre.
    parterreStemFirstVertex = static_cast<GLint>(litVertices.size() / 8);
    auto appendPlantVertex = [&litVertices](Vec3 point, Vec3 normal)
    {
        litVertices.insert(litVertices.end(), {
            point.x, point.y, point.z,
            normal.x, normal.y, normal.z, 0, 0
        });
    };
    auto appendPlantTriangle = [&appendPlantVertex](
        Vec3 a, Vec3 b, Vec3 c, Vec3 normal)
    {
        appendPlantVertex(a, normal);
        appendPlantVertex(b, normal);
        appendPlantVertex(c, normal);
    };
    auto appendPlantStem = [&appendPlantTriangle](const Vec3& flower)
    {
        float x = flower.x;
        float z = flower.z;
        appendPlantTriangle({x - 0.025f, 0, z},
                            {x + 0.025f, 0, z},
                            {x + 0.025f, 0.17f, z}, {0, 0, 1});
        appendPlantTriangle({x - 0.025f, 0, z},
                            {x + 0.025f, 0.17f, z},
                            {x - 0.025f, 0.17f, z}, {0, 0, 1});
        appendPlantTriangle({x, 0, z - 0.025f},
                            {x, 0, z + 0.025f},
                            {x, 0.17f, z + 0.025f}, {1, 0, 0});
        appendPlantTriangle({x, 0, z - 0.025f},
                            {x, 0.17f, z + 0.025f},
                            {x, 0.17f, z - 0.025f}, {1, 0, 0});
    };

    for (const auto& colorGroup : parterreFlowers)
    {
        for (const Vec3& flower : colorGroup)
        {
            appendPlantStem(flower);
        }
    }
    parterreStemVertexCount = static_cast<GLsizei>(litVertices.size() / 8) -
        parterreStemFirstVertex;

    // Four raised petals make each blossom legible at walking height. All
    // blossoms of one color share a draw range rather than separate draws.
    auto appendPetalTriangle = [&appendPlantTriangle](Vec3 a, Vec3 b, Vec3 c)
    {
        Vec3 normal = normalize(cross(b - a, c - a));
        if (normal.y < 0)
        {
            Vec3 previousB = b;
            b = c;
            c = previousB;
            normal = normalize(cross(b - a, c - a));
        }
        appendPlantTriangle(a, b, c, normal);
    };
    auto appendPlantBlossom = [&appendPetalTriangle](const Vec3& flower)
    {
        Vec3 crown{flower.x, 0.27f, flower.z};

        for (int petal = 0; petal < 4; ++petal)
        {
            float angle = 0.5f * pi * petal;
            Vec3 tip{flower.x + 0.34f * std::cos(angle), 0.16f,
                     flower.z + 0.34f * std::sin(angle)};
            Vec3 left{flower.x + 0.15f * std::cos(angle - 0.55f), 0.19f,
                      flower.z + 0.15f * std::sin(angle - 0.55f)};
            Vec3 right{flower.x + 0.15f * std::cos(angle + 0.55f), 0.19f,
                       flower.z + 0.15f * std::sin(angle + 0.55f)};
            appendPetalTriangle(crown, tip, left);
            appendPetalTriangle(crown, right, tip);
        }
    };

    for (std::size_t color = 0; color < parterreFlowers.size(); ++color)
    {
        parterreFlowerFirstVertex[color] = static_cast<GLint>(litVertices.size() / 8);

        for (const Vec3& flower : parterreFlowers[color])
        {
            appendPlantBlossom(flower);
        }

        parterreFlowerVertexCount[color] =
            static_cast<GLsizei>(litVertices.size() / 8) -
            parterreFlowerFirstVertex[color];
    }

    // Two narrow approach rows alternate restrained yellow and purple blocks.
    // Geometry is shared by the left and right strips.
    std::array<std::vector<Vec3>, 2> approachFlowers;
    for (int sample = 0; sample <= 80; ++sample)
    {
        float z = -30.0f + 0.75f * sample;
        int color = (sample / 8) % 2;
        approachFlowers[color].push_back({-0.45f, 0, z});
        approachFlowers[color].push_back({0.45f, 0, z});
    }

    approachStemFirstVertex = static_cast<GLint>(litVertices.size() / 8);
    for (const auto& colorGroup : approachFlowers)
    {
        for (const Vec3& flower : colorGroup)
        {
            appendPlantStem(flower);
        }
    }
    approachStemVertexCount = static_cast<GLsizei>(litVertices.size() / 8) -
        approachStemFirstVertex;

    for (std::size_t color = 0; color < approachFlowers.size(); ++color)
    {
        approachFlowerFirstVertex[color] = static_cast<GLint>(litVertices.size() / 8);
        for (const Vec3& flower : approachFlowers[color])
        {
            appendPlantBlossom(flower);
        }
        approachFlowerVertexCount[color] =
            static_cast<GLsizei>(litVertices.size() / 8) -
            approachFlowerFirstVertex[color];
    }

    // One curved quadrant is rotated four times around the circular plaza.
    constexpr float gardenStart = 27.0f * pi / 180.0f;
    constexpr float gardenEnd = 63.0f * pi / 180.0f;
    gardenLawnFirstVertex = static_cast<GLint>(litVertices.size() / 8);
    appendAnnulus(litVertices, 25.5f, 33.0f, gardenStart, gardenEnd, gardenSegments);

    gardenInnerBandFirstVertex = static_cast<GLint>(litVertices.size() / 8);
    appendAnnulus(litVertices, 25.5f, 26.5f, gardenStart, gardenEnd, gardenSegments);

    // One soil bed is framed by a shallow stone rim, repeated in all four gardens.
    gardenSoilFirstVertex = static_cast<GLint>(litVertices.size() / 8);
    constexpr float flowerStart = 37.0f * pi / 180.0f;
    constexpr float flowerEnd = 53.0f * pi / 180.0f;
    appendAnnulus(litVertices, 29.0f, 31.0f, flowerStart, flowerEnd, bedSegments);

    gardenBorderFirstVertex = static_cast<GLint>(litVertices.size() / 8);
    appendAnnulus(litVertices, 28.93f, 29.13f, flowerStart, flowerEnd, bedSegments, 0.09f);
    appendAnnulus(litVertices, 30.87f, 31.07f, flowerStart, flowerEnd, bedSegments, 0.09f);
    appendAnnulus(litVertices, 28.93f, 31.07f, flowerStart, flowerStart + 0.006f,
                  1, 0.09f);
    appendAnnulus(litVertices, 28.93f, 31.07f, flowerEnd - 0.006f, flowerEnd,
                  1, 0.09f);

    for (int segment = 0; segment < bedSegments; ++segment)
    {
        float angle = flowerStart + (flowerEnd - flowerStart) * segment / bedSegments;
        float nextAngle = flowerStart + (flowerEnd - flowerStart) * (segment + 1) / bedSegments;
        float middle = (angle + nextAngle) * 0.5f;

        appendBedWall(litVertices, bedPoint(31.07f, angle), bedPoint(31.07f, nextAngle),
                      {std::cos(middle), 0, std::sin(middle)});
        appendBedWall(litVertices, bedPoint(28.93f, nextAngle), bedPoint(28.93f, angle),
                      {-std::cos(middle), 0, -std::sin(middle)});
    }

    appendBedWall(litVertices, bedPoint(28.93f, flowerStart),
                  bedPoint(31.07f, flowerStart),
                  {std::sin(flowerStart), 0, -std::cos(flowerStart)});
    appendBedWall(litVertices, bedPoint(31.07f, flowerEnd),
                  bedPoint(28.93f, flowerEnd),
                  {-std::sin(flowerEnd), 0, std::cos(flowerEnd)});
    gardenBorderVertexCount = static_cast<GLsizei>(litVertices.size() / 8) -
        gardenBorderFirstVertex;

    // Five shallow pointed petals make a readable low-poly blossom in one draw.
    flowerBlossomFirstVertex = static_cast<GLint>(litVertices.size() / 8);

    for (int petal = 0; petal < 5; ++petal)
    {
        float angle = 2.0f * pi * petal / 5.0f;
        Vec3 left{0.12f * std::cos(angle - 0.42f), 0.02f,
                  0.12f * std::sin(angle - 0.42f)};
        Vec3 right{0.12f * std::cos(angle + 0.42f), 0.02f,
                   0.12f * std::sin(angle + 0.42f)};
        Vec3 tip{0.5f * std::cos(angle), 0, 0.5f * std::sin(angle)};
        Vec3 ridge{0.27f * std::cos(angle), 0.30f, 0.27f * std::sin(angle)};
        const Vec3 faces[] = {
            left, right, tip,
            left, ridge, right,
            right, ridge, tip,
            tip, ridge, left
        };

        for (int triangle = 0; triangle < 4; ++triangle)
        {
            const Vec3* face = faces + triangle * 3;
            Vec3 normal = normalize(cross(face[1] - face[0], face[2] - face[0]));

            for (int vertex = 0; vertex < 3; ++vertex)
            {
                Vec3 position = face[vertex];
                litVertices.insert(litVertices.end(), {
                    position.x, position.y, position.z,
                    normal.x, normal.y, normal.z, 0, 0
                });
            }
        }
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(litVertices.size() * sizeof(float)),
        litVertices.data(),
        GL_STATIC_DRAW
    );
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
        reinterpret_cast<const void*>(3 * sizeof(float))
    );
    glEnableVertexAttribArray(1);
    // Planar XZ UVs cover the plaza's top face from 0 to 1.
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
        reinterpret_cast<const void*>(6 * sizeof(float))
    );
    glEnableVertexAttribArray(2);
}

Mesh::~Mesh()
{
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void Mesh::drawGround() const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Mesh::drawCube() const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 6, 36);
}

void Mesh::drawSphere() const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, sphereFirstVertex, sphereVertexCount);
}

void Mesh::drawPointedTip() const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, tipFirstVertex, 18);
}

void Mesh::drawCircularPlaza() const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, circularPlazaFirstVertex, circleSegments * 6);
}

void Mesh::drawCircularBorder() const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, circularBorderFirstVertex, circleSegments * 12);
}

void Mesh::drawUnitDisk() const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, unitDiskFirstVertex, circleSegments * 3);
}

void Mesh::drawUnitRing() const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, unitRingFirstVertex, circleSegments * 6);
}

void Mesh::drawUnitCylinderWall() const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, unitCylinderWallFirstVertex, circleSegments * 6);
}

void Mesh::drawFountainJets() const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, fountainJetFirstVertex, fountainJetVertexCount);
}

void Mesh::drawUnitCapsule() const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, unitCapsuleFirstVertex, unitCapsuleVertexCount);
}

void Mesh::drawCeremonialCourt() const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, ceremonialCourtFirstVertex,
                 ceremonialCourtVertexCount);
}

void Mesh::drawParterreStems() const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, parterreStemFirstVertex,
                 parterreStemVertexCount);
}

void Mesh::drawParterreFlowers(int colorIndex) const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, parterreFlowerFirstVertex[colorIndex],
                 parterreFlowerVertexCount[colorIndex]);
}

void Mesh::drawApproachFlowerStems() const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, approachStemFirstVertex,
                 approachStemVertexCount);
}

void Mesh::drawApproachFlowers(int colorIndex) const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, approachFlowerFirstVertex[colorIndex],
                 approachFlowerVertexCount[colorIndex]);
}

void Mesh::drawGardenLawn() const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, gardenLawnFirstVertex, gardenSegments * 6);
}

void Mesh::drawGardenInnerBand() const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, gardenInnerBandFirstVertex, gardenSegments * 6);
}

void Mesh::drawGardenSoil() const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, gardenSoilFirstVertex, bedSegments * 6);
}

void Mesh::drawGardenBedBorder() const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, gardenBorderFirstVertex, gardenBorderVertexCount);
}

void Mesh::drawFlowerBlossom() const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, flowerBlossomFirstVertex, 60);
}
