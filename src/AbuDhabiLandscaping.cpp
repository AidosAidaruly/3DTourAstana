#include "AbuDhabiLandscaping.h"

#include <array>
#include <cmath>
#include <vector>

namespace
{
    constexpr int curveSegments = 32;
    constexpr int shrubSegments = 8;

    struct Oval
    {
        float x;
        float z;
        float radiusX;
        float radiusZ;
    };

    struct Flower
    {
        float x;
        float z;
        int color;
    };

    // These occupy bare ground, with clearance from the forecourt, east path,
    // sidewalks and the two storefront wings.
    constexpr std::array<Oval, 2> lawns{{
        {-171.0f, 65.0f, 14.0f, 18.0f},
        {-82.0f, 72.0f, 5.5f, 9.5f}
    }};
    constexpr std::array<Oval, 2> flowerbeds{{
        {-171.0f, 65.0f, 6.0f, 4.0f},
        {-82.0f, 72.0f, 3.0f, 4.0f}
    }};

    Vec3 ovalPoint(const Oval& oval, float angle, float y)
    {
        return {oval.x + oval.radiusX * std::cos(angle), y,
                oval.z + oval.radiusZ * std::sin(angle)};
    }

    GLuint appendVertex(std::vector<float>& vertices, Vec3 point, Vec3 normal)
    {
        GLuint index = static_cast<GLuint>(vertices.size() / 8);
        vertices.insert(vertices.end(), {
            point.x, point.y, point.z,
            normal.x, normal.y, normal.z,
            0.0f, 0.0f
        });
        return index;
    }

    void appendTriangle(std::vector<float>& vertices,
                        std::vector<GLuint>& indices,
                        Vec3 a, Vec3 b, Vec3 c, bool faceUp = false)
    {
        Vec3 normal = normalize(cross(b - a, c - a));

        if (faceUp && normal.y < 0.0f)
        {
            Vec3 previous = b;
            b = c;
            c = previous;
            normal = normalize(cross(b - a, c - a));
        }

        GLuint first = appendVertex(vertices, a, normal);
        appendVertex(vertices, b, normal);
        appendVertex(vertices, c, normal);
        indices.insert(indices.end(), {first, first + 1, first + 2});
    }

    void appendDisk(std::vector<float>& vertices, std::vector<GLuint>& indices,
                    const Oval& oval, float y)
    {
        Vec3 center{oval.x, y, oval.z};

        for (int segment = 0; segment < curveSegments; ++segment)
        {
            float start = 2.0f * pi * segment / curveSegments;
            float end = 2.0f * pi * (segment + 1) / curveSegments;
            appendTriangle(vertices, indices, center,
                           ovalPoint(oval, start, y),
                           ovalPoint(oval, end, y), true);
        }
    }

    void appendBorder(std::vector<float>& vertices,
                      std::vector<GLuint>& indices,
                      const Oval& oval, float width, float top,
                      float innerBase, float outerBase)
    {
        Oval outer{oval.x, oval.z, oval.radiusX + width,
                   oval.radiusZ + width};

        for (int segment = 0; segment < curveSegments; ++segment)
        {
            float start = 2.0f * pi * segment / curveSegments;
            float end = 2.0f * pi * (segment + 1) / curveSegments;
            Vec3 innerStart = ovalPoint(oval, start, top);
            Vec3 innerEnd = ovalPoint(oval, end, top);
            Vec3 outerStart = ovalPoint(outer, start, top);
            Vec3 outerEnd = ovalPoint(outer, end, top);
            appendTriangle(vertices, indices, innerStart, outerStart,
                           outerEnd, true);
            appendTriangle(vertices, indices, innerStart, outerEnd,
                           innerEnd, true);

            Vec3 innerStartLow = ovalPoint(oval, start, innerBase);
            Vec3 innerEndLow = ovalPoint(oval, end, innerBase);
            Vec3 outerStartLow = ovalPoint(outer, start, outerBase);
            Vec3 outerEndLow = ovalPoint(outer, end, outerBase);
            appendTriangle(vertices, indices, innerStartLow, innerStart,
                           innerEnd);
            appendTriangle(vertices, indices, innerStartLow, innerEnd,
                           innerEndLow);
            appendTriangle(vertices, indices, outerStartLow, outerEnd,
                           outerStart);
            appendTriangle(vertices, indices, outerStartLow, outerEndLow,
                           outerEnd);
        }
    }

    void appendShrub(std::vector<float>& vertices,
                     std::vector<GLuint>& indices, float x, float z)
    {
        constexpr std::array<float, 3> radii{{0.52f, 0.44f, 0.18f}};
        constexpr std::array<float, 3> heights{{0.055f, 0.40f, 0.65f}};

        for (int level = 0; level < 2; ++level)
        {
            for (int segment = 0; segment < shrubSegments; ++segment)
            {
                float start = 2.0f * pi * segment / shrubSegments;
                float end = 2.0f * pi * (segment + 1) / shrubSegments;
                auto point = [&](int ring, float angle)
                {
                    return Vec3{x + radii[ring] * std::cos(angle),
                                heights[ring],
                                z + radii[ring] * std::sin(angle)};
                };

                Vec3 a = point(level, start);
                Vec3 b = point(level + 1, start);
                Vec3 c = point(level + 1, end);
                Vec3 d = point(level, end);
                appendTriangle(vertices, indices, a, b, c);
                appendTriangle(vertices, indices, a, c, d);
            }
        }

        for (int segment = 0; segment < shrubSegments; ++segment)
        {
            float start = 2.0f * pi * segment / shrubSegments;
            float end = 2.0f * pi * (segment + 1) / shrubSegments;
            Vec3 a{x + radii[2] * std::cos(start), heights[2],
                   z + radii[2] * std::sin(start)};
            Vec3 b{x + radii[2] * std::cos(end), heights[2],
                   z + radii[2] * std::sin(end)};
            appendTriangle(vertices, indices, {x, 0.69f, z}, a, b, true);
        }
    }

    void appendStem(std::vector<float>& vertices,
                    std::vector<GLuint>& indices, const Flower& flower)
    {
        for (float angle : {0.0f, pi * 0.5f})
        {
            float dx = 0.025f * std::cos(angle);
            float dz = 0.025f * std::sin(angle);
            Vec3 a{flower.x - dx, 0.07f, flower.z - dz};
            Vec3 b{flower.x + dx, 0.07f, flower.z + dz};
            Vec3 c{flower.x + dx, 0.34f, flower.z + dz};
            Vec3 d{flower.x - dx, 0.34f, flower.z - dz};
            appendTriangle(vertices, indices, a, b, c);
            appendTriangle(vertices, indices, a, c, d);
        }
    }

    void appendBlossom(std::vector<float>& vertices,
                       std::vector<GLuint>& indices, const Flower& flower)
    {
        for (int petal = 0; petal < 5; ++petal)
        {
            float angle = 2.0f * pi * petal / 5.0f;
            auto edge = [&](float offset)
            {
                return Vec3{
                    flower.x + 0.20f * std::cos(angle + offset),
                    0.36f,
                    flower.z + 0.20f * std::sin(angle + offset)
                };
            };
            appendTriangle(vertices, indices,
                           {flower.x, 0.39f, flower.z},
                           edge(-0.36f), edge(0.36f), true);
        }
    }
}

AbuDhabiLandscaping::AbuDhabiLandscaping()
{
    trees.reserve(16);

    auto addTree = [&](float x, float z, std::size_t index)
    {
        constexpr std::array<float, 3> scales{{0.78f, 0.84f, 0.88f}};
        constexpr std::array<float, 4> colorVariation{{-0.045f, 0.025f,
                                                        0.05f, -0.015f}};
        trees.emplace_back(Vec3{x, 0, z}, scales[index % scales.size()],
                           colorVariation[index % colorVariation.size()]);
    };

    for (int row = 0; row < 6; ++row)
    {
        addTree(-216.0f, 52.0f + row * 20.0f, trees.size());
        addTree(-181.0f + row * 22.0f, 197.0f, trees.size());
    }

    for (int row = 0; row < 4; ++row)
    {
        addTree(-47.0f, 88.0f + row * 22.0f, trees.size());
    }

    std::vector<float> vertices;
    std::vector<GLuint> indices;
    auto recordRange = [&](IndexRange& range, std::size_t start)
    {
        range.firstIndex = start;
        range.count = static_cast<GLsizei>(indices.size() - start);
    };

    std::size_t start = indices.size();
    for (const Oval& lawn : lawns)
    {
        appendDisk(vertices, indices, lawn, 0.055f);
    }
    recordRange(lawnRange, start);

    start = indices.size();
    for (const Oval& lawn : lawns)
    {
        appendBorder(vertices, indices, lawn, 0.25f,
                     0.085f, 0.055f, 0.0f);
    }
    recordRange(lawnBorderRange, start);

    start = indices.size();
    for (const Oval& bed : flowerbeds)
    {
        appendDisk(vertices, indices, bed, 0.068f);
    }
    recordRange(soilRange, start);

    start = indices.size();
    for (const Oval& bed : flowerbeds)
    {
        appendBorder(vertices, indices, bed, 0.18f,
                     0.10f, 0.068f, 0.055f);
    }
    recordRange(soilBorderRange, start);

    start = indices.size();
    for (float x : {-179.0f, -163.0f})
    {
        for (float z : {56.0f, 65.0f, 74.0f})
        {
            appendShrub(vertices, indices, x, z);
        }
    }
    for (float x : {-85.0f, -79.0f})
    {
        for (float z : {67.0f, 77.0f})
        {
            appendShrub(vertices, indices, x, z);
        }
    }
    recordRange(shrubRange, start);

    std::vector<Flower> flowers;
    flowers.reserve(25);
    for (int row = 0; row < 4; ++row)
    {
        for (int column = 0; column < 4; ++column)
        {
            flowers.push_back({-175.2f + column * 2.8f,
                               62.3f + row * 1.8f,
                               (row + column) % 3});
        }
    }
    for (int row = 0; row < 3; ++row)
    {
        for (int column = 0; column < 3; ++column)
        {
            flowers.push_back({-83.7f + column * 1.7f,
                               70.0f + row * 2.0f,
                               (row + 2 * column) % 3});
        }
    }

    start = indices.size();
    for (const Flower& flower : flowers)
    {
        appendStem(vertices, indices, flower);
    }
    recordRange(stemRange, start);

    std::array<IndexRange*, 3> flowerRanges{{
        &redFlowerRange, &yellowFlowerRange, &orangeFlowerRange
    }};
    for (int color = 0; color < 3; ++color)
    {
        start = indices.size();
        for (const Flower& flower : flowers)
        {
            if (flower.color == color)
            {
                appendBlossom(vertices, indices, flower);
            }
        }
        recordRange(*flowerRanges[color], start);
    }

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

AbuDhabiLandscaping::~AbuDhabiLandscaping()
{
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void AbuDhabiLandscaping::drawRange(const IndexRange& range, Vec3 color,
                                    GLint objectColorLocation) const
{
    glUniform3f(objectColorLocation, color.x, color.y, color.z);
    glDrawElements(GL_TRIANGLES, range.count, GL_UNSIGNED_INT,
                   reinterpret_cast<void*>(range.firstIndex * sizeof(GLuint)));
}

void AbuDhabiLandscaping::render(const Mesh& mesh, GLint modelLocation,
                                 GLint objectColorLocation,
                                 GLint useGrassLocation) const
{
    const Mat4 identity = modelMatrix({0, 0, 0}, 0, {1, 1, 1});
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, identity.data());
    glBindVertexArray(vao);

    glUniform1i(useGrassLocation, GL_TRUE);
    drawRange(lawnRange, {0.34f, 0.48f, 0.32f}, objectColorLocation);
    drawRange(shrubRange, {0.25f, 0.43f, 0.28f}, objectColorLocation);
    glUniform1i(useGrassLocation, GL_FALSE);

    drawRange(lawnBorderRange, {0.75f, 0.76f, 0.73f}, objectColorLocation);
    drawRange(soilRange, {0.30f, 0.25f, 0.20f}, objectColorLocation);
    drawRange(soilBorderRange, {0.72f, 0.73f, 0.71f}, objectColorLocation);
    drawRange(stemRange, {0.26f, 0.42f, 0.24f}, objectColorLocation);
    drawRange(redFlowerRange, {0.67f, 0.25f, 0.24f}, objectColorLocation);
    drawRange(yellowFlowerRange, {0.80f, 0.66f, 0.31f}, objectColorLocation);
    drawRange(orangeFlowerRange, {0.80f, 0.46f, 0.27f}, objectColorLocation);

    glBindVertexArray(0);

    for (const Tree& tree : trees)
    {
        tree.render(mesh, modelLocation, objectColorLocation);
    }
}
