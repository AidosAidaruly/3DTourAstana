#include "Tree.h"

Tree::Tree(Vec3 position, float scale)
{
    // Apply the same parent position and uniform scale to every local component.
    trunkModel = modelMatrix(
        position + Vec3{0, 2.2f, 0} * scale,
        0,
        Vec3{0.65f, 4.4f, 0.65f} * scale
    );

    const std::array<Vec3, 4> foliageOffsets{{
        {-1.0f, 4.6f, 0},
        {1.0f, 4.9f, 0.4f},
        {0, 5.2f, -1.0f},
        {0.1f, 6.3f, 0}
    }};
    const std::array<float, 4> foliageRadii{{1.8f, 1.9f, 1.75f, 1.7f}};

    // Overlapping clusters connect to the trunk; the highest reaches 8 units.
    for (std::size_t i = 0; i < foliageModels.size(); ++i)
    {
        float radius = foliageRadii[i] * scale;
        foliageModels[i] = modelMatrix(
            position + foliageOffsets[i] * scale,
            0,
            {radius, radius, radius}
        );
    }
}

void Tree::render(
    const Mesh& mesh,
    GLint modelLocation,
    GLint objectColorLocation
) const
{
    const float trunkColor[3] = {0.36f, 0.22f, 0.12f};
    const std::array<Vec3, 4> foliageColors{{
        {0.19f, 0.40f, 0.20f},
        {0.22f, 0.44f, 0.23f},
        {0.18f, 0.38f, 0.21f},
        {0.24f, 0.46f, 0.24f}
    }};

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, trunkModel.data());
    glUniform3fv(objectColorLocation, 1, trunkColor);
    mesh.drawCube();

    for (std::size_t i = 0; i < foliageModels.size(); ++i)
    {
        const Vec3& color = foliageColors[i];
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, foliageModels[i].data());
        glUniform3f(objectColorLocation, color.x, color.y, color.z);
        mesh.drawSphere();
    }
}
