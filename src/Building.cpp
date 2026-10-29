#include "Building.h"

Building::Building(Vec3 position, Vec3 dimensions, Vec3 color, WindowLayout layout)
    : model(modelMatrix(position, 0, dimensions)), color(color)
{
    const int rows = layout.rows;
    const int frontColumns = layout.frontColumns;
    const int sideColumns = layout.sideColumns;
    constexpr float windowThickness = 0.08f;
    constexpr float wallOffset = 0.06f;

    // Equal top and bottom margins keep the floor grid centered on the facade.
    float rowSpacing = (dimensions.y - 2.0f * layout.verticalMargin) / rows;
    float frontSpacing = dimensions.x / frontColumns;
    float sideSpacing = dimensions.z / sideColumns;
    float windowHeight = rowSpacing * layout.heightFraction;

    windowModels.reserve(2 * rows * (frontColumns + sideColumns));

    // Mirror the same grid onto opposite facades.
    for (int side = -1; side <= 1; side += 2)
    {
        float direction = static_cast<float>(side);

        for (int row = 0; row < rows; ++row)
        {
            float y = position.y - dimensions.y * 0.5f + layout.verticalMargin
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
                    {frontSpacing * layout.widthFraction, windowHeight, windowThickness}
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
                    {windowThickness, windowHeight, sideSpacing * layout.widthFraction}
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
