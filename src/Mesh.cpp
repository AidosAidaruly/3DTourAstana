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

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(meshVertices.size() * sizeof(float)),
        meshVertices.data(),
        GL_STATIC_DRAW
    );
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
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
