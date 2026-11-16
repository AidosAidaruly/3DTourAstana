#include "KhanDistrictTowers.h"

#include "KhanDistrict.h"

#include <array>
#include <cmath>
#include <vector>

namespace
{
    using TowerSpec = KhanDistrict::TowerSite;
    constexpr const auto& towers = KhanDistrict::towerSites;

    struct Placement
    {
        Vec3 position;
        float cosine;
        float sine;
    };

    Placement placementFor(const TowerSpec& tower)
    {
        // Local +Z is the entrance-facing side, aimed toward Khan Shatyr.
        float angle = std::atan2(-tower.position.x, -420.0f - tower.position.z);
        return {tower.position, std::cos(angle), std::sin(angle)};
    }

    Vec3 toWorld(const Placement& placement, Vec3 local)
    {
        return {
            placement.position.x + placement.cosine * local.x + placement.sine * local.z,
            placement.position.y + local.y,
            placement.position.z - placement.sine * local.x + placement.cosine * local.z
        };
    }

    GLuint appendVertex(std::vector<float>& vertices, Vec3 position,
                        Vec3 normal, float u = 0.0f, float v = 0.0f)
    {
        GLuint index = static_cast<GLuint>(vertices.size() / 8);
        vertices.insert(vertices.end(), {
            position.x, position.y, position.z,
            normal.x, normal.y, normal.z,
            u, v
        });
        return index;
    }

    void appendQuad(std::vector<float>& vertices, std::vector<GLuint>& indices,
                    const Placement& placement, Vec3 a, Vec3 b, Vec3 c, Vec3 d,
                    float u0 = 0.0f, float u1 = 0.0f,
                    float v0 = 0.0f, float v1 = 0.0f)
    {
        Vec3 worldA = toWorld(placement, a);
        Vec3 worldB = toWorld(placement, b);
        Vec3 worldC = toWorld(placement, c);
        Vec3 worldD = toWorld(placement, d);
        Vec3 normal = normalize(cross(worldB - worldA, worldC - worldA));
        GLuint first = appendVertex(vertices, worldA, normal, u0, v0);
        appendVertex(vertices, worldB, normal, u0, v1);
        appendVertex(vertices, worldC, normal, u1, v1);
        appendVertex(vertices, worldD, normal, u1, v0);
        indices.insert(indices.end(), {
            first, first + 1, first + 2,
            first, first + 2, first + 3
        });
    }

    void appendBox(std::vector<float>& vertices, std::vector<GLuint>& indices,
                   const Placement& placement, Vec3 lower, Vec3 upper)
    {
        float x0 = lower.x, y0 = lower.y, z0 = lower.z;
        float x1 = upper.x, y1 = upper.y, z1 = upper.z;

        appendQuad(vertices, indices, placement,
            {x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1});
        appendQuad(vertices, indices, placement,
            {x1, y0, z0}, {x0, y0, z0}, {x0, y1, z0}, {x1, y1, z0});
        appendQuad(vertices, indices, placement,
            {x1, y0, z1}, {x1, y0, z0}, {x1, y1, z0}, {x1, y1, z1});
        appendQuad(vertices, indices, placement,
            {x0, y0, z0}, {x0, y0, z1}, {x0, y1, z1}, {x0, y1, z0});
        appendQuad(vertices, indices, placement,
            {x0, y1, z1}, {x1, y1, z1}, {x1, y1, z0}, {x0, y1, z0});
        appendQuad(vertices, indices, placement,
            {x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1});
    }

    std::array<Vec3, 8> footprint(float width, float depth, float chamfer)
    {
        float x = width * 0.5f;
        float z = depth * 0.5f;
        return {{
            {x - chamfer, 0, z}, {-x + chamfer, 0, z},
            {-x, 0, z - chamfer}, {-x, 0, -z + chamfer},
            {-x + chamfer, 0, -z}, {x - chamfer, 0, -z},
            {x, 0, -z + chamfer}, {x, 0, z - chamfer}
        }};
    }

    void appendBand(std::vector<float>& vertices, std::vector<GLuint>& indices,
                    const Placement& placement,
                    const std::array<Vec3, 8>& outline,
                    float bottom, float top, bool facade)
    {
        float totalPerimeter = 0.0f;

        for (std::size_t side = 0; side < outline.size(); ++side)
        {
            Vec3 edge = outline[(side + 1) % outline.size()] - outline[side];
            totalPerimeter += std::sqrt(dot(edge, edge));
        }

        float bayCount = std::round(totalPerimeter / 2.25f);
        float perimeter = 0.0f;

        for (std::size_t side = 0; side < outline.size(); ++side)
        {
            Vec3 a = outline[side];
            Vec3 b = outline[(side + 1) % outline.size()];
            Vec3 edge = b - a;
            float length = std::sqrt(dot(edge, edge));
            float u0 = facade ? perimeter * bayCount / totalPerimeter : 0.0f;
            float u1 = facade ? (perimeter + length) * bayCount / totalPerimeter : 0.0f;
            appendQuad(vertices, indices, placement,
                {a.x, bottom, a.z}, {a.x, top, a.z},
                {b.x, top, b.z}, {b.x, bottom, b.z},
                u0, u1, (bottom - 3.6f) / 3.4f, (top - 3.6f) / 3.4f);
            perimeter += length;
        }
    }

    void appendTop(std::vector<float>& vertices, std::vector<GLuint>& indices,
                   const Placement& placement,
                   const std::array<Vec3, 8>& outline, float height)
    {
        Vec3 center = toWorld(placement, {0, height, 0});

        for (std::size_t side = 0; side < outline.size(); ++side)
        {
            Vec3 a = toWorld(placement, {
                outline[side].x, height, outline[side].z
            });
            Vec3 b = toWorld(placement, {
                outline[(side + 1) % outline.size()].x, height,
                outline[(side + 1) % outline.size()].z
            });
            GLuint first = appendVertex(vertices, center, {0, 1, 0});
            appendVertex(vertices, b, {0, 1, 0});
            appendVertex(vertices, a, {0, 1, 0});
            indices.insert(indices.end(), {first, first + 1, first + 2});
        }
    }

    void appendFacade(std::vector<float>& vertices, std::vector<GLuint>& indices,
                      const TowerSpec& tower)
    {
        appendBand(vertices, indices, placementFor(tower),
                   footprint(tower.width, tower.depth, 1.4f),
                   0.20f, tower.height - 1.05f, true);
    }

    void appendEntranceGlass(std::vector<float>& vertices,
                             std::vector<GLuint>& indices, const TowerSpec& tower)
    {
        Placement placement = placementFor(tower);
        float front = tower.depth * 0.5f;

        for (float side : {-1.0f, 1.0f})
        {
            float x0 = side < 0 ? -2.77f : 0.07f;
            float x1 = side < 0 ? -0.07f : 2.77f;
            appendBox(vertices, indices, placement,
                      {x0, 0.27f, front + 0.08f},
                      {x1, 3.47f, front + 0.15f});
        }
    }

    void appendFrames(std::vector<float>& vertices, std::vector<GLuint>& indices,
                      const TowerSpec& tower)
    {
        Placement placement = placementFor(tower);
        float front = tower.depth * 0.5f;
        auto outline = footprint(tower.width, tower.depth, 1.4f);

        appendBand(vertices, indices, placement, outline,
                   tower.height - 1.05f, tower.height - 0.65f, false);

        for (const Vec3& corner : outline)
        {
            appendBox(vertices, indices, placement,
                {corner.x - 0.07f, 3.65f, corner.z - 0.07f},
                {corner.x + 0.07f, tower.height - 1.05f, corner.z + 0.07f});
        }

        // A shallow projecting portal distinguishes the entrance from the facade grid.
        for (float x : {-2.88f, 0.0f, 2.88f})
        {
            appendBox(vertices, indices, placement,
                {x - 0.09f, 0.20f, front + 0.04f},
                {x + 0.09f, 3.65f, front + 0.22f});
        }

        appendBox(vertices, indices, placement,
            {-3.0f, 3.53f, front + 0.04f},
            {3.0f, 3.70f, front + 0.24f});
        appendBox(vertices, indices, placement,
            {-3.30f, 3.70f, front - 0.30f},
            {3.30f, 3.92f, front + 0.95f});

        for (float x : {-0.23f, 0.23f})
        {
            appendBox(vertices, indices, placement,
                {x - 0.035f, 1.25f, front + 0.16f},
                {x + 0.035f, 1.80f, front + 0.23f});
        }
    }

    void appendRoof(std::vector<float>& vertices, std::vector<GLuint>& indices,
                    const TowerSpec& tower)
    {
        Placement placement = placementFor(tower);
        auto outline = footprint(tower.width + 0.5f, tower.depth + 0.5f, 1.5f);
        appendBand(vertices, indices, placement, outline,
                   tower.height - 0.65f, tower.height - 0.45f, false);
        appendTop(vertices, indices, placement, outline, tower.height - 0.45f);
        appendBox(vertices, indices, placement,
            {-tower.width * 0.5f + 1.6f, tower.height - 0.45f,
             -tower.depth * 0.5f + 1.6f},
            {tower.width * 0.5f - 1.6f, tower.height,
             tower.depth * 0.5f - 1.6f});
    }

    void appendBase(std::vector<float>& vertices, std::vector<GLuint>& indices,
                    const TowerSpec& tower)
    {
        Placement placement = placementFor(tower);
        auto outline = footprint(tower.width + 0.3f, tower.depth + 0.3f, 1.5f);
        appendBand(vertices, indices, placement, outline, 0.0f, 0.20f, false);
    }
}

KhanDistrictTowers::KhanDistrictTowers()
{
    std::vector<float> vertices;
    std::vector<GLuint> indices;

    auto recordRange = [&](IndexRange& range, std::size_t start)
    {
        range.firstIndex = start;
        range.count = static_cast<GLsizei>(indices.size() - start);
    };

    std::size_t start = indices.size();
    for (const TowerSpec& tower : towers)
    {
        appendFacade(vertices, indices, tower);
    }
    recordRange(facadeRange, start);

    start = indices.size();
    for (const TowerSpec& tower : towers)
    {
        appendEntranceGlass(vertices, indices, tower);
    }
    recordRange(entranceGlassRange, start);

    start = indices.size();
    for (const TowerSpec& tower : towers)
    {
        appendFrames(vertices, indices, tower);
    }
    recordRange(frameRange, start);

    start = indices.size();
    for (const TowerSpec& tower : towers)
    {
        appendRoof(vertices, indices, tower);
    }
    recordRange(roofRange, start);

    start = indices.size();
    for (const TowerSpec& tower : towers)
    {
        appendBase(vertices, indices, tower);
    }
    recordRange(baseRange, start);

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

KhanDistrictTowers::~KhanDistrictTowers()
{
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void KhanDistrictTowers::drawRange(const IndexRange& range, Vec3 color,
                                    GLint objectColorLocation) const
{
    glUniform3f(objectColorLocation, color.x, color.y, color.z);
    glDrawElements(GL_TRIANGLES, range.count, GL_UNSIGNED_INT,
                   reinterpret_cast<void*>(range.firstIndex * sizeof(GLuint)));
}

void KhanDistrictTowers::render(GLint modelLocation, GLint objectColorLocation,
                                GLint khanTowerFacadeLocation) const
{
    const Mat4 identity = modelMatrix({0, 0, 0}, 0, {1, 1, 1});
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, identity.data());
    glBindVertexArray(vao);

    glUniform1i(khanTowerFacadeLocation, GL_TRUE);
    drawRange(facadeRange, {0.32f, 0.50f, 0.61f}, objectColorLocation);
    glUniform1i(khanTowerFacadeLocation, GL_FALSE);

    drawRange(entranceGlassRange, {0.18f, 0.32f, 0.41f}, objectColorLocation);
    drawRange(frameRange, {0.80f, 0.84f, 0.85f}, objectColorLocation);
    drawRange(roofRange, {0.43f, 0.53f, 0.57f}, objectColorLocation);
    drawRange(baseRange, {0.65f, 0.68f, 0.68f}, objectColorLocation);

    glBindVertexArray(0);
}
