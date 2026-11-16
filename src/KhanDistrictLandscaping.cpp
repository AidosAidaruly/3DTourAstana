#include "KhanDistrictLandscaping.h"

#include "KhanDistrict.h"

#include <array>
#include <cmath>
#include <vector>

namespace
{
    constexpr int bedSegments = 16;
    constexpr int shrubSegments = 8;

    struct PlantingBed
    {
        Vec3 center;
        Vec3 outward;
    };

    Vec3 tangentTo(Vec3 outward)
    {
        return {-outward.z, 0, outward.x};
    }

    Vec3 ellipsePoint(const PlantingBed& bed, float radialRadius,
                      float tangentialRadius, float angle, float height)
    {
        Vec3 tangent = tangentTo(bed.outward);
        return bed.center +
            bed.outward * (radialRadius * std::sin(angle)) +
            tangent * (tangentialRadius * std::cos(angle)) +
            Vec3{0, height, 0};
    }

    GLuint appendVertex(std::vector<float>& vertices, Vec3 position, Vec3 normal)
    {
        GLuint index = static_cast<GLuint>(vertices.size() / 8);
        vertices.insert(vertices.end(), {
            position.x, position.y, position.z,
            normal.x, normal.y, normal.z,
            0.0f, 0.0f
        });
        return index;
    }

    void appendUpTriangle(std::vector<float>& vertices,
                          std::vector<GLuint>& indices, Vec3 a, Vec3 b, Vec3 c)
    {
        GLuint first = appendVertex(vertices, a, {0, 1, 0});
        appendVertex(vertices, b, {0, 1, 0});
        appendVertex(vertices, c, {0, 1, 0});

        if (cross(b - a, c - a).y > 0.0f)
        {
            indices.insert(indices.end(), {first, first + 1, first + 2});
        }
        else
        {
            indices.insert(indices.end(), {first, first + 2, first + 1});
        }
    }

    void appendUpQuad(std::vector<float>& vertices, std::vector<GLuint>& indices,
                      Vec3 a, Vec3 b, Vec3 c, Vec3 d)
    {
        appendUpTriangle(vertices, indices, a, b, c);
        appendUpTriangle(vertices, indices, a, c, d);
    }

    void appendBed(std::vector<float>& vertices, std::vector<GLuint>& indices,
                   const PlantingBed& bed)
    {
        Vec3 center = bed.center + Vec3{0, 0.055f, 0};

        for (int segment = 0; segment < bedSegments; ++segment)
        {
            float start = 2.0f * pi * segment / bedSegments;
            float end = 2.0f * pi * (segment + 1) / bedSegments;
            appendUpTriangle(vertices, indices, center,
                ellipsePoint(bed, 1.8f, 2.6f, start, 0.055f),
                ellipsePoint(bed, 1.8f, 2.6f, end, 0.055f));
        }
    }

    void appendBedBorder(std::vector<float>& vertices,
                         std::vector<GLuint>& indices, const PlantingBed& bed)
    {
        for (int segment = 0; segment < bedSegments; ++segment)
        {
            float start = 2.0f * pi * segment / bedSegments;
            float end = 2.0f * pi * (segment + 1) / bedSegments;
            appendUpQuad(vertices, indices,
                ellipsePoint(bed, 1.8f, 2.6f, start, 0.10f),
                ellipsePoint(bed, 2.0f, 2.8f, start, 0.10f),
                ellipsePoint(bed, 2.0f, 2.8f, end, 0.10f),
                ellipsePoint(bed, 1.8f, 2.6f, end, 0.10f));
        }
    }

    void appendRadialBuffer(std::vector<float>& vertices,
                            std::vector<GLuint>& indices,
                            Vec3 outward, float across)
    {
        Vec3 tangent = tangentTo(outward);
        float left = across - 1.25f;
        float right = across + 1.25f;
        appendUpQuad(vertices, indices,
            outward * 145.0f + tangent * left + Vec3{0, 0.055f, 0},
            outward * 145.0f + tangent * right + Vec3{0, 0.055f, 0},
            outward * 157.0f + tangent * right + Vec3{0, 0.055f, 0},
            outward * 157.0f + tangent * left + Vec3{0, 0.055f, 0});
    }

    void appendShrub(std::vector<float>& vertices, std::vector<GLuint>& indices,
                     Vec3 center, float radius, float height)
    {
        constexpr std::array<float, 3> radiusFractions{{1.0f, 0.78f, 0.30f}};
        constexpr std::array<float, 3> heightFractions{{0.08f, 0.55f, 0.88f}};

        for (int ring = 0; ring < 2; ++ring)
        {
            for (int segment = 0; segment < shrubSegments; ++segment)
            {
                float start = 2.0f * pi * segment / shrubSegments;
                float end = 2.0f * pi * (segment + 1) / shrubSegments;
                auto point = [&](int level, float angle)
                {
                    return center + Vec3{
                        radius * radiusFractions[level] * std::cos(angle),
                        height * heightFractions[level],
                        radius * radiusFractions[level] * std::sin(angle)
                    };
                };

                Vec3 a = point(ring, start);
                Vec3 b = point(ring + 1, start);
                Vec3 c = point(ring + 1, end);
                Vec3 d = point(ring, end);
                Vec3 normal = normalize(cross(b - a, c - a));
                GLuint first = appendVertex(vertices, a, normal);
                appendVertex(vertices, b, normal);
                appendVertex(vertices, c, normal);
                appendVertex(vertices, d, normal);
                indices.insert(indices.end(), {
                    first, first + 1, first + 2,
                    first, first + 2, first + 3
                });
            }
        }

        Vec3 top = center + Vec3{0, height, 0};

        for (int segment = 0; segment < shrubSegments; ++segment)
        {
            float start = 2.0f * pi * segment / shrubSegments;
            float end = 2.0f * pi * (segment + 1) / shrubSegments;
            Vec3 a = center + Vec3{
                radius * 0.30f * std::cos(start), height * 0.88f,
                radius * 0.30f * std::sin(start)
            };
            Vec3 b = center + Vec3{
                radius * 0.30f * std::cos(end), height * 0.88f,
                radius * 0.30f * std::sin(end)
            };
            appendUpTriangle(vertices, indices, top, a, b);
        }
    }
}

KhanDistrictLandscaping::KhanDistrictLandscaping(Vec3 position)
    : model(modelMatrix(position, 0, {1, 1, 1}))
{
    constexpr std::array<float, 12> outerTreeAngles{{
        25, 42, 60, 75, 105, 120, 138, 155, 215, 235, 305, 325
    }};
    constexpr std::array<float, 3> outerTreeScales{{0.88f, 0.94f, 0.90f}};
    constexpr std::array<float, 4> clusterAngles{{42, 138, 235, 305}};
    trees.reserve(22);

    for (std::size_t i = 0; i < outerTreeAngles.size(); ++i)
    {
        float angle = outerTreeAngles[i] * pi / 180.0f;
        trees.emplace_back(
            position + Vec3{160.0f * std::cos(angle), 0,
                            160.0f * std::sin(angle)},
            outerTreeScales[i % outerTreeScales.size()]
        );
    }

    for (float degrees : clusterAngles)
    {
        float angle = degrees * pi / 180.0f;
        trees.emplace_back(
            position + Vec3{168.0f * std::cos(angle), 0,
                            168.0f * std::sin(angle)},
            0.74f
        );
    }

    std::vector<PlantingBed> beds;
    beds.reserve(KhanDistrict::towerSites.size() * 2);

    for (const KhanDistrict::TowerSite& site : KhanDistrict::towerSites)
    {
        Vec3 relative = site.position - position;
        Vec3 outward = normalize(relative);
        Vec3 tangent = tangentTo(outward);
        float treeSide = site.position.x < 0 ? 1.0f : -1.0f;
        trees.emplace_back(position + outward * 84.0f +
                           tangent * (treeSide * 10.5f), 0.62f);

        for (float side : {-1.0f, 1.0f})
        {
            beds.push_back({outward * 91.0f + tangent * (side * 7.5f), outward});
        }
    }

    std::vector<float> vertices;
    std::vector<GLuint> indices;
    auto recordRange = [&](IndexRange& range, std::size_t start)
    {
        range.firstIndex = start;
        range.count = static_cast<GLsizei>(indices.size() - start);
    };

    std::size_t start = indices.size();
    for (const PlantingBed& bed : beds)
    {
        appendBed(vertices, indices, bed);
    }
    recordRange(bedRange, start);

    start = indices.size();
    for (const PlantingBed& bed : beds)
    {
        appendBedBorder(vertices, indices, bed);
    }
    recordRange(borderRange, start);

    start = indices.size();
    for (std::size_t site : {2u, 3u})
    {
        Vec3 outward = normalize(KhanDistrict::towerSites[site].position - position);

        for (float side : {-1.0f, 1.0f})
        {
            appendRadialBuffer(vertices, indices, outward, side * 15.5f);
        }
    }
    recordRange(bufferRange, start);

    start = indices.size();
    for (const PlantingBed& bed : beds)
    {
        Vec3 tangent = tangentTo(bed.outward);

        for (int shrub = -1; shrub <= 1; ++shrub)
        {
            Vec3 center = bed.center + tangent * (shrub * 1.3f) +
                bed.outward * (shrub == 0 ? 0.25f : -0.15f);
            appendShrub(vertices, indices, center,
                        shrub == 0 ? 0.62f : 0.54f, 0.65f);
        }
    }

    for (std::size_t site : {2u, 3u})
    {
        Vec3 outward = normalize(KhanDistrict::towerSites[site].position - position);
        Vec3 tangent = tangentTo(outward);

        for (float side : {-1.0f, 1.0f})
        {
            for (float radius : {147.0f, 151.0f, 155.0f})
            {
                Vec3 center = outward * radius + tangent * (side * 15.5f);
                appendShrub(vertices, indices, center, 0.55f, 0.58f);
            }
        }
    }
    recordRange(shrubRange, start);

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

KhanDistrictLandscaping::~KhanDistrictLandscaping()
{
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void KhanDistrictLandscaping::drawRange(const IndexRange& range, Vec3 color,
                                        GLint objectColorLocation) const
{
    glUniform3f(objectColorLocation, color.x, color.y, color.z);
    glDrawElements(GL_TRIANGLES, range.count, GL_UNSIGNED_INT,
                   reinterpret_cast<void*>(range.firstIndex * sizeof(GLuint)));
}

void KhanDistrictLandscaping::render(const Mesh& mesh, GLint modelLocation,
                                     GLint objectColorLocation,
                                     GLint useGrassLocation) const
{
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model.data());
    glBindVertexArray(vao);

    glUniform1i(useGrassLocation, GL_TRUE);
    drawRange(bedRange, {0.31f, 0.42f, 0.29f}, objectColorLocation);
    drawRange(bufferRange, {0.35f, 0.47f, 0.32f}, objectColorLocation);
    drawRange(shrubRange, {0.23f, 0.41f, 0.24f}, objectColorLocation);
    glUniform1i(useGrassLocation, GL_FALSE);
    drawRange(borderRange, {0.68f, 0.69f, 0.65f}, objectColorLocation);

    glBindVertexArray(0);

    for (const Tree& tree : trees)
    {
        tree.render(mesh, modelLocation, objectColorLocation);
    }
}
