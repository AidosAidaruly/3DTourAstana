#include "StreetLight.h"

StreetLight::StreetLight(Vec3 position, float rotationDegrees, float scale)
    : metalParts{{
        partModelMatrix(position, rotationDegrees,
                        Vec3{0, 0.08f, 0} * scale,
                        Vec3{0.38f, 0.16f, 0.38f} * scale),
        partModelMatrix(position, rotationDegrees,
                        Vec3{0, 3.16f, 0} * scale,
                        Vec3{0.14f, 6.16f, 0.14f} * scale),
        partModelMatrix(position, rotationDegrees,
                        Vec3{0, 6.20f, -0.28f} * scale,
                        Vec3{0.12f, 0.12f, 0.62f} * scale),
        partModelMatrix(position, rotationDegrees,
                        Vec3{0, 6.28f, -0.55f} * scale,
                        Vec3{0.48f, 0.16f, 0.52f} * scale)
    }},
    diffuser(partModelMatrix(
        position, rotationDegrees,
        Vec3{0, 6.15f, -0.55f} * scale,
        Vec3{0.42f, 0.09f, 0.46f} * scale
    ))
{
}

void StreetLight::render(const Mesh& mesh, GLint modelLocation, GLint objectColorLocation) const
{
    glUniform3f(objectColorLocation, 0.62f, 0.65f, 0.67f);

    for (const Mat4& part : metalParts)
    {
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, part.data());
        mesh.drawCube();
    }

    // The diffuser is only visible glass-colored geometry; it emits no light.
    glUniform3f(objectColorLocation, 0.86f, 0.88f, 0.84f);
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, diffuser.data());
    mesh.drawCube();
}
