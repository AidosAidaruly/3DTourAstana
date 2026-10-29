#include "Bench.h"

Bench::Bench(Vec3 position, float rotationDegrees)
{
    // Four legs and two rear uprights carry the seat and backrest.
    for (int side = 0; side < 2; ++side)
    {
        float x = side == 0 ? -1.02f : 1.02f;

        for (int depth = 0; depth < 2; ++depth)
        {
            float z = depth == 0 ? -0.25f : 0.25f;
            frameParts[side * 2 + depth] = partModelMatrix(
                position, rotationDegrees, {x, 0.215f, z}, {0.10f, 0.43f, 0.10f}
            );
        }

        frameParts[4 + side] = partModelMatrix(
            position, rotationDegrees, {x, 0.705f, 0.31f}, {0.10f, 0.55f, 0.10f}
        );
        frameParts[6 + side] = partModelMatrix(
            position, rotationDegrees, {x, 0.40f, 0}, {0.10f, 0.08f, 0.68f}
        );
    }

    // Three slats span a 0.55-meter seat; the top is 0.48 meters above paving.
    for (int slat = 0; slat < 3; ++slat)
    {
        float z = -0.185f + slat * 0.185f;
        woodenSlats[slat] = partModelMatrix(
            position, rotationDegrees, {0, 0.4475f, z}, {2.40f, 0.065f, 0.18f}
        );
    }

    // Four taller slats make the backrest visible without exceeding human scale.
    for (int slat = 0; slat < 4; ++slat)
    {
        float y = 0.56f + slat * 0.12f;
        woodenSlats[3 + slat] = partModelMatrix(
            position, rotationDegrees, {0, y, 0.32f}, {2.40f, 0.10f, 0.065f}
        );
    }
}

void Bench::render(const Mesh& mesh, GLint modelLocation, GLint objectColorLocation) const
{
    glUniform3f(objectColorLocation, 0.20f, 0.23f, 0.25f);

    for (const Mat4& part : frameParts)
    {
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, part.data());
        mesh.drawCube();
    }

    glUniform3f(objectColorLocation, 0.43f, 0.29f, 0.18f);

    for (const Mat4& slat : woodenSlats)
    {
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, slat.data());
        mesh.drawCube();
    }
}
