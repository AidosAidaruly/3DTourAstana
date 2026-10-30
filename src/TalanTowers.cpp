#include "TalanTowers.h"

#include <cmath>
#include <vector>

namespace
{
    constexpr int cornerSegments = 6;

    void appendVertex(
        std::vector<float>& vertices,
        Vec3 position,
        Vec3 normal,
        float facadeU = 0.0f
    )
    {
        // Match the shared shader's position, normal, and UV vertex layout.
        vertices.insert(vertices.end(), {
            position.x, position.y, position.z,
            normal.x, normal.y, normal.z,
            facadeU, 0.0f
        });
    }

    GLsizei appendRoundedTower(
        std::vector<float>& vertices,
        float width,
        float depth,
        float cornerRadius,
        float roofDrop,
        bool highOnLeft,
        int bayCount,
        float edgeThickness
    )
    {
        std::vector<Vec3> outline;
        std::vector<Vec3> normals;
        std::vector<float> perimeterDistances;
        outline.reserve(4 * (cornerSegments + 1));
        normals.reserve(outline.capacity());
        perimeterDistances.reserve(outline.capacity());

        const float halfWidth = width * 0.5f;
        const float halfDepth = depth * 0.5f;
        const float roofSlope = (highOnLeft ? -roofDrop : roofDrop) / width;
        const Vec3 roofNormal = normalize(Vec3{-roofSlope, 1.0f, 0.0f});
        auto roofHeight = [roofDrop, halfWidth, width, highOnLeft](float x)
        {
            float inwardDistance = highOnLeft ? x + halfWidth : halfWidth - x;
            return 1.0f - roofDrop * inwardDistance / width;
        };

        for (int corner = 0; corner < 4; ++corner)
        {
            float centerX = (corner == 0 || corner == 3)
                ? halfWidth - cornerRadius : -halfWidth + cornerRadius;
            float centerZ = corner < 2
                ? halfDepth - cornerRadius : -halfDepth + cornerRadius;

            for (int segment = 0; segment <= cornerSegments; ++segment)
            {
                float angle = (corner + static_cast<float>(segment) / cornerSegments)
                    * pi * 0.5f;
                float x = std::cos(angle);
                float z = std::sin(angle);
                outline.push_back({
                    centerX + cornerRadius * x, 0.0f,
                    centerZ + cornerRadius * z
                });
                normals.push_back({x, 0.0f, z});
            }
        }

        perimeterDistances.push_back(0.0f);

        for (std::size_t i = 1; i < outline.size(); ++i)
        {
            Vec3 step = outline[i] - outline[i - 1];
            perimeterDistances.push_back(
                perimeterDistances.back() + std::sqrt(dot(step, step))
            );
        }

        Vec3 closingStep = outline.front() - outline.back();
        float perimeter = perimeterDistances.back() + std::sqrt(dot(closingStep, closingStep));

        for (std::size_t i = 0; i < outline.size(); ++i)
        {
            std::size_t next = (i + 1) % outline.size();
            Vec3 lower = outline[i];
            Vec3 nextLower = outline[next];
            Vec3 upper = lower + Vec3{0.0f, roofHeight(lower.x), 0.0f};
            Vec3 nextUpper = nextLower + Vec3{0.0f, roofHeight(nextLower.x), 0.0f};
            float u = perimeterDistances[i] * bayCount / perimeter;
            float nextU = next == 0 ? static_cast<float>(bayCount)
                : perimeterDistances[next] * bayCount / perimeter;

            appendVertex(vertices, lower, normals[i], u);
            appendVertex(vertices, nextUpper, normals[next], nextU);
            appendVertex(vertices, nextLower, normals[next], nextU);
            appendVertex(vertices, lower, normals[i], u);
            appendVertex(vertices, upper, normals[i], u);
            appendVertex(vertices, nextUpper, normals[next], nextU);
        }

        GLsizei sideCount = static_cast<GLsizei>(outline.size() * 6);

        // Each shallow roof slopes inward, as in the two framing tower silhouettes.
        for (std::size_t i = 0; i < outline.size(); ++i)
        {
            std::size_t next = (i + 1) % outline.size();
            Vec3 upper = outline[i] + Vec3{0.0f, roofHeight(outline[i].x), 0.0f};
            Vec3 nextUpper = outline[next] + Vec3{0.0f, roofHeight(outline[next].x), 0.0f};

            appendVertex(vertices, {0.0f, roofHeight(0.0f), 0.0f}, roofNormal);
            appendVertex(vertices, nextUpper, roofNormal);
            appendVertex(vertices, upper, roofNormal);
        }

        // A slim metal lip follows the sloping roof without covering the glass.
        for (std::size_t i = 0; i < outline.size(); ++i)
        {
            std::size_t next = (i + 1) % outline.size();
            Vec3 lower = outline[i] + normals[i] * 0.04f;
            Vec3 nextLower = outline[next] + normals[next] * 0.04f;
            lower.y = roofHeight(outline[i].x) - edgeThickness;
            nextLower.y = roofHeight(outline[next].x) - edgeThickness;
            Vec3 upper = lower + Vec3{0.0f, edgeThickness, 0.0f};
            Vec3 nextUpper = nextLower + Vec3{0.0f, edgeThickness, 0.0f};

            appendVertex(vertices, lower, normals[i]);
            appendVertex(vertices, nextUpper, normals[next]);
            appendVertex(vertices, nextLower, normals[next]);
            appendVertex(vertices, lower, normals[i]);
            appendVertex(vertices, upper, normals[i]);
            appendVertex(vertices, nextUpper, normals[next]);
        }

        return sideCount;
    }

    constexpr std::array<Vec3, 8> storefrontCorners{{
        {-24.5f, 0, 10.5f},
        {-24.5f, 0, 15.0f},
        {-2.5f, 0, 24.0f},
        {-2.5f, 0, 23.2f},
        {2.5f, 0, 23.2f},
        {2.5f, 0, 24.0f},
        {24.5f, 0, 15.0f},
        {24.5f, 0, 10.5f}
    }};

    float canopyUndersideHeight(float z);

    void appendStorefrontGlass(std::vector<float>& vertices)
    {
        for (std::size_t i = 0; i + 1 < storefrontCorners.size(); ++i)
        {
            Vec3 start = storefrontCorners[i];
            Vec3 end = storefrontCorners[i + 1];
            Vec3 direction = end - start;
            Vec3 normal = normalize(Vec3{-direction.z, 0, direction.x});
            Vec3 lowerStart = start + Vec3{0, 0.12f, 0};
            Vec3 lowerEnd = end + Vec3{0, 0.12f, 0};
            Vec3 upperStart = start + Vec3{0, canopyUndersideHeight(start.z), 0};
            Vec3 upperEnd = end + Vec3{0, canopyUndersideHeight(end.z), 0};

            appendVertex(vertices, lowerStart, normal);
            appendVertex(vertices, lowerEnd, normal);
            appendVertex(vertices, upperEnd, normal);
            appendVertex(vertices, lowerStart, normal);
            appendVertex(vertices, upperEnd, normal);
            appendVertex(vertices, upperStart, normal);
        }
    }

    constexpr std::array<Vec3, 5> canopyOutline{{
        {-25, 0, 11}, {-25, 0, 19}, {0, 0, 27},
        {25, 0, 19}, {25, 0, 11}
    }};

    float canopyTopHeight(float z)
    {
        // Both roof planes rise toward the projecting corner, keeping the edge slim.
        return 4.0f + 0.45f * (z - 11.0f) / 16.0f;
    }

    float canopyUndersideHeight(float z)
    {
        return z <= 11.0f ? 3.5f : 3.5f + 0.55f * (z - 11.0f) / 16.0f;
    }

    void appendGalleryCanopy(std::vector<float>& vertices)
    {
        // Two angled roof wings meet at the elevated entrance corner.
        Vec3 topNormal = normalize(Vec3{0, 1, -0.45f / 16.0f});
        Vec3 undersideNormal = normalize(Vec3{0, -1, 0.55f / 16.0f});

        for (std::size_t i = 1; i + 1 < canopyOutline.size(); ++i)
        {
            appendVertex(vertices, {
                canopyOutline[0].x, canopyTopHeight(canopyOutline[0].z), canopyOutline[0].z
            }, topNormal);
            appendVertex(vertices, {
                canopyOutline[i].x, canopyTopHeight(canopyOutline[i].z), canopyOutline[i].z
            }, topNormal);
            appendVertex(vertices, {
                canopyOutline[i + 1].x, canopyTopHeight(canopyOutline[i + 1].z),
                canopyOutline[i + 1].z
            }, topNormal);
        }

        for (std::size_t i = 1; i + 1 < canopyOutline.size(); ++i)
        {
            appendVertex(vertices, {
                canopyOutline[0].x, canopyUndersideHeight(canopyOutline[0].z),
                canopyOutline[0].z
            }, undersideNormal);
            appendVertex(vertices, {
                canopyOutline[i + 1].x, canopyUndersideHeight(canopyOutline[i + 1].z),
                canopyOutline[i + 1].z
            }, undersideNormal);
            appendVertex(vertices, {
                canopyOutline[i].x, canopyUndersideHeight(canopyOutline[i].z),
                canopyOutline[i].z
            }, undersideNormal);
        }

        // The closing rear edge meets the rectangular roof, so it needs no face.
        for (std::size_t i = 0; i + 1 < canopyOutline.size(); ++i)
        {
            Vec3 start = canopyOutline[i];
            Vec3 end = canopyOutline[i + 1];
            Vec3 direction = end - start;
            Vec3 normal = normalize(Vec3{-direction.z, 0, direction.x});
            Vec3 lowerStart{start.x, canopyUndersideHeight(start.z), start.z};
            Vec3 lowerEnd{end.x, canopyUndersideHeight(end.z), end.z};
            Vec3 upperStart{start.x, canopyTopHeight(start.z), start.z};
            Vec3 upperEnd{end.x, canopyTopHeight(end.z), end.z};

            appendVertex(vertices, lowerStart, normal);
            appendVertex(vertices, lowerEnd, normal);
            appendVertex(vertices, upperEnd, normal);
            appendVertex(vertices, lowerStart, normal);
            appendVertex(vertices, upperEnd, normal);
            appendVertex(vertices, upperStart, normal);
        }
    }
}

TalanTowers::TalanTowers(Vec3 position)
    : towerModels{{
        modelMatrix({position.x - 15.0f, position.y + 4.0f, position.z}, 0, {1, 56, 1}),
        modelMatrix({position.x + 15.0f, position.y + 4.0f, position.z}, 0, {1, 46, 1})
    }},
    galleryModel(modelMatrix(position, 0, {1, 1, 1})),
    podiumModel(modelMatrix(
        {position.x, position.y + 1.75f, position.z - 3.0f}, 0, {49, 3.5f, 27}
    )),
    roofModel(modelMatrix(
        {position.x, position.y + 3.75f, position.z - 3.0f}, 0, {50, 0.5f, 28}
    ))
{
    std::vector<float> vertices;
    vertices.reserve((2 * 4 * (cornerSegments + 1) * 15 + 100) * 8);

    towerRanges[0].firstVertex = 0;
    towerRanges[0].sideVertexCount = appendRoundedTower(
        vertices, 19.0f, 19.0f, 2.8f, 4.0f / 56.0f, true, 40, 0.35f / 56.0f
    );
    towerRanges[0].roofVertexCount = 4 * (cornerSegments + 1) * 3;
    towerRanges[0].edgeVertexCount = 4 * (cornerSegments + 1) * 6;

    towerRanges[1].firstVertex = static_cast<GLint>(vertices.size() / 8);
    towerRanges[1].sideVertexCount = appendRoundedTower(
        vertices, 18.0f, 18.0f, 2.3f, 3.0f / 46.0f, false, 38, 0.35f / 46.0f
    );
    towerRanges[1].roofVertexCount = 4 * (cornerSegments + 1) * 3;
    towerRanges[1].edgeVertexCount = 4 * (cornerSegments + 1) * 6;

    galleryGlassFirstVertex = static_cast<GLint>(vertices.size() / 8);
    appendStorefrontGlass(vertices);
    galleryGlassVertexCount = static_cast<GLsizei>(vertices.size() / 8) -
        galleryGlassFirstVertex;

    canopyFirstVertex = static_cast<GLint>(vertices.size() / 8);
    appendGalleryCanopy(vertices);
    canopyTopVertexCount = static_cast<GLsizei>((canopyOutline.size() - 2) * 3);
    canopyUndersideVertexCount = canopyTopVertexCount;
    canopyFasciaVertexCount = static_cast<GLsizei>(vertices.size() / 8) -
        canopyFirstVertex - canopyTopVertexCount - canopyUndersideVertexCount;

    const std::array<int, 7> divisions{{2, 9, 1, 4, 1, 9, 2}};

    for (std::size_t segment = 0; segment < divisions.size(); ++segment)
    {
        Vec3 start = storefrontCorners[segment];
        Vec3 end = storefrontCorners[segment + 1];
        Vec3 direction = end - start;
        Vec3 normal = normalize(Vec3{-direction.z, 0, direction.x});
        float length = std::sqrt(dot(direction, direction));
        float rotation = -std::atan2(direction.z, direction.x) * 180.0f / pi;

        for (int bay = 0; bay < divisions[segment]; ++bay)
        {
            float fraction = static_cast<float>(bay) / divisions[segment];
            Vec3 point = start + direction * fraction;
            float height = canopyUndersideHeight(point.z);
            Vec3 center = position + point + normal * 0.08f +
                Vec3{0, height * 0.5f, 0};
            float frameWidth = (segment == 1 || segment == 5) && bay % 3 == 0
                ? 0.22f : 0.14f;
            storefrontFrames.push_back(modelMatrix(
                center, 0, {frameWidth, height, frameWidth}
            ));
        }

        Vec3 middle = position + (start + end) * 0.5f + normal * 0.08f;
        const std::array<float, 3> railHeights{{0.10f, 2.68f, 3.44f}};
        const std::array<float, 3> railThicknesses{{0.20f, 0.10f, 0.12f}};

        for (std::size_t rail = 0; rail < railHeights.size(); ++rail)
        {
            if (segment == 3 && rail == 1)
            {
                continue; // The entrance has its own taller door header.
            }

            Vec3 center = middle + Vec3{0, railHeights[rail], 0};
            storefrontFrames.push_back(modelMatrix(
                center, rotation, {length, railThicknesses[rail], 0.14f}
            ));
        }
    }

    Vec3 last = storefrontCorners.back();
    storefrontFrames.push_back(modelMatrix(
        position + last + Vec3{0, 1.75f, 0}, 0, {0.14f, 3.5f, 0.14f}
    ));

    // A shallow recessed entry separates the doors from the angled storefront.
    storefrontFrames.push_back(modelMatrix(
        position + Vec3{0, 2.86f, 23.34f}, 0, {5.2f, 0.15f, 0.18f}
    ));
    for (float side : {-2.5f, 2.5f})
    {
        storefrontFrames.push_back(modelMatrix(
            position + Vec3{side, 1.43f, 23.34f}, 0, {0.20f, 2.86f, 0.18f}
        ));
    }

    for (int door = 0; door < 4; ++door)
    {
        float x = -1.875f + 1.25f * door;
        entranceDoors.push_back(modelMatrix(
            position + Vec3{x, 1.45f, 23.24f}, 0, {1.16f, 2.66f, 0.08f}
        ));
        float handleOffset = (door % 2 == 0) ? 0.40f : -0.40f;
        doorHandles.push_back(modelMatrix(
            position + Vec3{x + handleOffset, 1.12f, 23.35f}, 0,
            {0.04f, 0.34f, 0.05f}
        ));
    }

    const std::array<float, 4> columnX{{-21, -7, 7, 21}};

    for (std::size_t i = 0; i < columnX.size(); ++i)
    {
        float x = columnX[i];
        float frontZ = 27.0f - 8.0f * std::abs(x) / 25.0f;
        float columnZ = frontZ - 0.60f;
        float height = canopyUndersideHeight(columnZ);
        canopyColumns[i] = modelMatrix(
            position + Vec3{x, height * 0.5f, columnZ}, 0,
            {0.34f, height, 0.34f}
        );
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
        vertices.data(),
        GL_STATIC_DRAW
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

TalanTowers::~TalanTowers()
{
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void TalanTowers::render(
    const Mesh& mesh,
    GLint modelLocation,
    GLint objectColorLocation,
    GLint talanFacadeLocation
) const
{
    auto drawCube = [&](const Mat4& model, Vec3 color)
    {
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model.data());
        glUniform3f(objectColorLocation, color.x, color.y, color.z);
        mesh.drawCube();
    };

    // The rear volume supports both towers; the front gallery opens into a V.
    glUniform1i(talanFacadeLocation, 2);
    drawCube(podiumModel, {0.39f, 0.53f, 0.61f});
    glUniform1i(talanFacadeLocation, 0);
    drawCube(roofModel, {0.72f, 0.73f, 0.71f});

    glBindVertexArray(vao);
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, galleryModel.data());
    glUniform3f(objectColorLocation, 0.39f, 0.53f, 0.61f);
    glUniform1i(talanFacadeLocation, 3);
    glDrawArrays(GL_TRIANGLES, galleryGlassFirstVertex, galleryGlassVertexCount);

    glUniform1i(talanFacadeLocation, 0);
    glUniform3f(objectColorLocation, 0.73f, 0.73f, 0.70f);
    glDrawArrays(GL_TRIANGLES, canopyFirstVertex, canopyTopVertexCount);
    glUniform3f(objectColorLocation, 0.33f, 0.36f, 0.38f);
    glDrawArrays(
        GL_TRIANGLES,
        canopyFirstVertex + canopyTopVertexCount,
        canopyUndersideVertexCount
    );
    glUniform3f(objectColorLocation, 0.79f, 0.77f, 0.71f);
    glDrawArrays(
        GL_TRIANGLES,
        canopyFirstVertex + canopyTopVertexCount + canopyUndersideVertexCount,
        canopyFasciaVertexCount
    );
    glBindVertexArray(0);

    glUniform1i(talanFacadeLocation, 3);

    for (const Mat4& door : entranceDoors)
    {
        drawCube(door, {0.17f, 0.31f, 0.40f});
    }

    glUniform1i(talanFacadeLocation, 0);

    for (const Mat4& frame : storefrontFrames)
    {
        drawCube(frame, {0.72f, 0.69f, 0.60f});
    }

    for (const Mat4& column : canopyColumns)
    {
        drawCube(column, {0.77f, 0.73f, 0.64f});
    }

    for (const Mat4& handle : doorHandles)
    {
        drawCube(handle, {0.77f, 0.73f, 0.64f});
    }

    const std::array<Vec3, 2> glassColors{{
        {0.31f, 0.47f, 0.58f},
        {0.38f, 0.51f, 0.61f}
    }};

    glBindVertexArray(vao);

    for (std::size_t i = 0; i < towerRanges.size(); ++i)
    {
        const TowerRange& range = towerRanges[i];
        const Vec3& color = glassColors[i];
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, towerModels[i].data());
        glUniform3f(objectColorLocation, color.x, color.y, color.z);
        glUniform1i(talanFacadeLocation, 1);
        glDrawArrays(GL_TRIANGLES, range.firstVertex, range.sideVertexCount);

        glUniform1i(talanFacadeLocation, 0);
        glUniform3f(objectColorLocation, 0.54f, 0.60f, 0.63f);
        glDrawArrays(
            GL_TRIANGLES,
            range.firstVertex + range.sideVertexCount,
            range.roofVertexCount
        );

        glUniform3f(objectColorLocation, 0.78f, 0.72f, 0.59f);
        glDrawArrays(
            GL_TRIANGLES,
            range.firstVertex + range.sideVertexCount + range.roofVertexCount,
            range.edgeVertexCount
        );
    }

    glBindVertexArray(0);
}
