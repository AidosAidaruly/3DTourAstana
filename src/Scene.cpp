#include "Scene.h"

#include <vector>

namespace
{
    GLuint createPlazaTexture()
    {
        constexpr int size = 64;
        constexpr int tileSize = 32;
        std::vector<unsigned char> pixels(size * size * 3);

        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                int tileX = x / tileSize;
                int tileY = y / tileSize;
                bool grout = x % tileSize == 0 || y % tileSize == 0;
                int shade = grout ? 210 : 242 + (tileX * 7 + tileY * 11) % 14;
                std::size_t index = static_cast<std::size_t>((y * size + x) * 3);

                for (int channel = 0; channel < 3; ++channel)
                {
                    pixels[index + channel] = static_cast<unsigned char>(shade);
                }
            }
        }

        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGB8, size, size, 0,
            GL_RGB, GL_UNSIGNED_BYTE, pixels.data()
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        return texture;
    }
}

Scene::Scene(const std::filesystem::path& shaderDirectory)
    : shader(shaderDirectory)
{
    viewLocation = shader.uniformLocation("view");
    projectionLocation = shader.uniformLocation("projection");
    modelLocation = shader.uniformLocation("model");
    useObjectColorLocation = shader.uniformLocation("useObjectColor");
    objectColorLocation = shader.uniformLocation("objectColor");
    lightDirectionLocation = shader.uniformLocation("lightDirection");
    ambientStrengthLocation = shader.uniformLocation("ambientStrength");
    diffuseStrengthLocation = shader.uniformLocation("diffuseStrength");
    useTextureLocation = shader.uniformLocation("useTexture");
    plazaTextureLocation = shader.uniformLocation("plazaTexture");
    textureRepeatLocation = shader.uniformLocation("textureRepeat");
    plazaTexture = createPlazaTexture();
}

Scene::~Scene()
{
    glDeleteTextures(1, &plazaTexture);
}

void Scene::render(const Mat4& view, const Mat4& projection) const
{
    const float plazaColor[3] = {0.80f, 0.80f, 0.78f};
    const float walkwayColor[3] = {0.52f, 0.53f, 0.54f};

    shader.use();
    glUniform1i(useTextureLocation, GL_FALSE);
    glUniform3f(lightDirectionLocation, lightDirection.x, lightDirection.y, lightDirection.z);
    glUniform1f(ambientStrengthLocation, ambientStrength);
    glUniform1f(diffuseStrengthLocation, diffuseStrength);
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, view.data());
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, projection.data());

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, groundModel.data());
    glUniform1i(useObjectColorLocation, GL_FALSE);
    mesh.drawGround();

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, plazaModel.data());
    glUniform1i(useObjectColorLocation, GL_TRUE);
    glUniform3fv(objectColorLocation, 1, plazaColor);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, plazaTexture);
    glUniform1i(plazaTextureLocation, 0);
    // Each image contains 2 x 2 tiles: 50 x 30 tiles across the 100 x 60 plaza.
    glUniform2f(textureRepeatLocation, 25.0f, 15.0f);
    glUniform1i(useTextureLocation, GL_TRUE);
    mesh.drawCube();
    glUniform1i(useTextureLocation, GL_FALSE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, walkwayModel.data());
    glUniform3fv(objectColorLocation, 1, walkwayColor);
    mesh.drawCube();

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, oppositeWalkwayModel.data());
    mesh.drawCube();

    baiterek.render(mesh, modelLocation, objectColorLocation);
    building.render(mesh, modelLocation, objectColorLocation);
    secondBuilding.render(mesh, modelLocation, objectColorLocation);

    for (const Tree& tree : trees)
    {
        tree.render(mesh, modelLocation, objectColorLocation);
    }
}
