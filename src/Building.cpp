#include "Building.h"

Building::Building(Vec3 position, Vec3 dimensions, Vec3 color)
    : model(modelMatrix(position, 0, dimensions)), color(color)
{
    constexpr int rows = 6;
    constexpr int frontColumns = 4;
    constexpr int sideColumns = 3;
    constexpr float windowThickness = 0.08f;
    constexpr float wallOffset = 0.06f;

    float rowSpacing = dimensions.y / rows;
    float frontSpacing = dimensions.x / frontColumns;
    float sideSpacing = dimensions.z / sideColumns;
    float windowHeight = rowSpacing * 0.5f;

    windowModels.reserve(2 * rows * (frontColumns + sideColumns));

    // Mirror the same grid onto opposite facades.
    for (int side = -1; side <= 1; side += 2)
    {
        float direction = static_cast<float>(side);

        for (int row = 0; row < rows; ++row)
        {
            float y = position.y - dimensions.y * 0.5f
                + (static_cast<float>(row) + 0.5f) * rowSpacing;

            // The inner face remains 0.02 units outside the wall to avoid z-fighting.
            for (int column = 0; column < frontColumns; ++column)
            {
                float x = position.x - dimensions.x * 0.5f
                    + (static_cast<float>(column) + 0.5f) * frontSpacing;
                float z = position.z + direction * (dimensions.z * 0.5f + wallOffset);

                windowModels.push_back(modelMatrix(
                    {x, y, z},
                    0,
                    {frontSpacing * 0.55f, windowHeight, windowThickness}
                ));
            }

            for (int column = 0; column < sideColumns; ++column)
            {
                float x = position.x + direction * (dimensions.x * 0.5f + wallOffset);
                float z = position.z - dimensions.z * 0.5f
                    + (static_cast<float>(column) + 0.5f) * sideSpacing;

                windowModels.push_back(modelMatrix(
                    {x, y, z},
                    0,
                    {windowThickness, windowHeight, sideSpacing * 0.55f}
                ));
            }
        }
    }
}

void Building::render(
    const Mesh& mesh,
    GLint modelLocation,
    GLint objectColorLocation
) const
{
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model.data());
    glUniform3f(objectColorLocation, color.x, color.y, color.z);
    mesh.drawCube();

    const float windowColor[3] = {0.12f, 0.20f, 0.28f};
    glUniform3fv(objectColorLocation, 1, windowColor);

    for (const Mat4& windowModel : windowModels)
    {
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, windowModel.data());
        mesh.drawCube();
    }
}
