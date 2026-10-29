#include "Scene.h"

#include <cstdint>
#include <vector>

namespace
{
    GLuint uploadRepeatingTexture(int size, const std::vector<unsigned char>& pixels)
    {
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


    std::uint32_t pavingHash(std::uint32_t x, std::uint32_t y)
    {
        std::uint32_t value = x * 374761393u + y * 668265263u;
        value = (value ^ (value >> 13)) * 1274126177u;
        return value ^ (value >> 16);
    }

    GLuint createPlazaTexture()
    {
        // A 12 x 12 meter patch contains 1.5 x 1 meter slabs.
        constexpr int size = 768;
        constexpr int slabWidth = 96;
        constexpr int slabHeight = 64;
        constexpr int jointWidth = 1;
        std::vector<unsigned char> pixels(size * size * 3);

        for (int y = 0; y < size; ++y)
        {
            int row = y / slabHeight;

            for (int x = 0; x < size; ++x)
            {
                // Quarter-slab offsets alternate and wrap at the texture boundaries.
                int shiftedX = (x + (row % 2) * (slabWidth / 4)) % size;
                int column = shiftedX / slabWidth;
                bool joint = shiftedX % slabWidth < jointWidth
                    || y % slabHeight < jointWidth;
                int slabVariation = static_cast<int>(pavingHash(column, row) % 9u) - 4;
                int grain = static_cast<int>(pavingHash(x, y) % 3u) - 1;
                int warmth = static_cast<int>(pavingHash(column + 31, row) % 3u);
                int shade = (joint ? 231 : 246 + slabVariation) + grain;
                std::size_t index = static_cast<std::size_t>((y * size + x) * 3);

                pixels[index] = static_cast<unsigned char>(shade);
                pixels[index + 1] = static_cast<unsigned char>(shade - warmth);
                pixels[index + 2] = static_cast<unsigned char>(shade - warmth * 2);
            }
        }

        return uploadRepeatingTexture(size, pixels);
    }

    GLuint createWalkwayTexture()
    {
        // One repeat covers 8 x 8 meters: stones measure 1 x 0.5 meters.
        constexpr int size = 512;
        constexpr int stoneWidth = 64;
        constexpr int stoneHeight = 32;
        constexpr int jointWidth = 2;
        std::vector<unsigned char> pixels(size * size * 3);

        for (int y = 0; y < size; ++y)
        {
            int row = y / stoneHeight;

            for (int x = 0; x < size; ++x)
            {
                // Wrap shifted rows so the texture joins seamlessly at its edges.
                int shiftedX = (x + (row % 2) * (stoneWidth / 2)) % size;
                int column = shiftedX / stoneWidth;
                bool joint = shiftedX % stoneWidth < jointWidth
                    || y % stoneHeight < jointWidth;
                int stoneVariation = static_cast<int>(pavingHash(column, row) % 17u) - 8;
                int grain = static_cast<int>(pavingHash(x, y) % 5u) - 2;
                int shade = (joint ? 196 : 235 + stoneVariation) + grain;
                std::size_t index = static_cast<std::size_t>((y * size + x) * 3);

                pixels[index] = static_cast<unsigned char>(shade);
                pixels[index + 1] = static_cast<unsigned char>(shade - 1);
                pixels[index + 2] = static_cast<unsigned char>(shade - 3);
            }
        }

        return uploadRepeatingTexture(size, pixels);
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
    goldenPanelsLocation = shader.uniformLocation("goldenPanels");
    shaftGlassLocation = shader.uniformLocation("shaftGlass");
    useTextureLocation = shader.uniformLocation("useTexture");
    plazaTextureLocation = shader.uniformLocation("plazaTexture");
    textureRepeatLocation = shader.uniformLocation("textureRepeat");
    plazaTexture = createPlazaTexture();
    walkwayTexture = createWalkwayTexture();
}

Scene::~Scene()
{
    glDeleteTextures(1, &plazaTexture);
    glDeleteTextures(1, &walkwayTexture);
}

void Scene::render(const Mat4& view, const Mat4& projection) const
{
    const float plazaColor[3] = {0.80f, 0.80f, 0.78f};
    const float walkwayColor[3] = {0.52f, 0.53f, 0.54f};

    shader.use();
    glUniform1i(goldenPanelsLocation, GL_FALSE);
    glUniform1i(shaftGlassLocation, GL_FALSE);
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
    // Repeat a 12-meter patch without stretching the 1.5 x 1 meter slabs.
    glUniform2f(textureRepeatLocation, 100.0f / 12.0f, 60.0f / 12.0f);
    glUniform1i(useTextureLocation, GL_TRUE);
    mesh.drawCube();
    glUniform1i(useTextureLocation, GL_FALSE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, walkwayModel.data());
    glUniform3fv(objectColorLocation, 1, walkwayColor);
    glBindTexture(GL_TEXTURE_2D, walkwayTexture);
    glUniform2f(textureRepeatLocation, 1.5f, 7.5f);
    glUniform1i(useTextureLocation, GL_TRUE);
    mesh.drawCube();

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, oppositeWalkwayModel.data());
    mesh.drawCube();
    glUniform1i(useTextureLocation, GL_FALSE);
    glBindTexture(GL_TEXTURE_2D, 0);

    baiterek.render(
        mesh, modelLocation, objectColorLocation, goldenPanelsLocation, shaftGlassLocation
    );
    building.render(mesh, modelLocation, objectColorLocation);
    secondBuilding.render(mesh, modelLocation, objectColorLocation);
    thirdBuilding.render(mesh, modelLocation, objectColorLocation);
    fourthBuilding.render(mesh, modelLocation, objectColorLocation);

    for (const Tree& tree : trees)
    {
        tree.render(mesh, modelLocation, objectColorLocation);
    }
}
