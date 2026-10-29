#include "Mesh.h"
#include "Math3D.h"

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
