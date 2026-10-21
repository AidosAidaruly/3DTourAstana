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
