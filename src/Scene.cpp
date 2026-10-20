#include "Scene.h"

Scene::Scene(const std::filesystem::path& shaderDirectory)
    : shader(shaderDirectory)
{
    viewLocation = shader.uniformLocation("view");
    projectionLocation = shader.uniformLocation("projection");
    modelLocation = shader.uniformLocation("model");
    useObjectColorLocation = shader.uniformLocation("useObjectColor");
    objectColorLocation = shader.uniformLocation("objectColor");
}

void Scene::render(const Mat4& view, const Mat4& projection) const
{
    const float plazaColor[3] = {0.75f, 0.75f, 0.75f};

    shader.use();
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, view.data());
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, projection.data());

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, groundModel.data());
    glUniform1i(useObjectColorLocation, GL_FALSE);
    mesh.drawGround();

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, plazaModel.data());
    glUniform1i(useObjectColorLocation, GL_TRUE);
    glUniform3fv(objectColorLocation, 1, plazaColor);
    mesh.drawCube();
}
