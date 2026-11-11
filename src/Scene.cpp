#include "Scene.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace
{
    struct PavingRectangle
    {
        float minX;
        float maxX;
        float minZ;
        float maxZ;
    };

    // The new route meets existing paved edges without covering the Baiterek
    // plaza or its camera-side walkway.
    constexpr std::array<PavingRectangle, 5> abuDhabiPaving{{
        {-153.0f, -93.0f, 43.0f, 83.0f},
        {-93.0f, -40.0f, 50.0f, 60.0f},
        {-50.0f, -40.0f, 30.0f, 50.0f},
        {-153.0f, -135.0f, 83.0f, 87.5f},
        {-111.0f, -93.0f, 83.0f, 87.5f}
    }};

    // The raised stone base supports both storefront wings and the recessed
    // central wall. Its front edge meets the landing and side paving.
    constexpr std::array<PavingRectangle, 3> abuDhabiRaisedBase{{
        {-155.0f, -135.0f, 87.5f, 101.0f},
        {-111.0f, -91.0f, 87.5f, 101.0f},
        {-135.0f, -111.0f, 92.0f, 101.0f}
    }};

    // Low stone edging follows the extended forecourt sides, leaving the
    // pedestrian route and central steps unobstructed.
    constexpr std::array<PavingRectangle, 8> abuDhabiBorders{{
        {-153.12f, -152.88f, 43.12f, 87.38f},
        {-153.0f, -93.0f, 42.88f, 43.12f},
        {-93.12f, -92.88f, 43.12f, 49.88f},
        {-93.12f, -92.88f, 60.12f, 87.38f},
        {-93.0f, -40.0f, 59.88f, 60.12f},
        {-93.0f, -50.0f, 49.88f, 50.12f},
        {-50.12f, -49.88f, 30.0f, 49.88f},
        {-40.12f, -39.88f, 30.0f, 50.0f}
    }};

    // Four connected streets frame the complex. The east street pauses for
    // the approved pedestrian route, which crosses at z = 50..60.
    constexpr std::array<PavingRectangle, 5> abuDhabiRoads{{
        {-205.0f, -58.0f, 28.0f, 38.0f},
        {-205.0f, -58.0f, 175.0f, 185.0f},
        {-205.0f, -195.0f, 38.0f, 175.0f},
        {-68.0f, -58.0f, 38.0f, 50.0f},
        {-68.0f, -58.0f, 60.0f, 175.0f}
    }};

    constexpr PavingRectangle abuDhabiCrossing{-68.0f, -58.0f, 50.0f, 60.0f};

    // Five-meter inner sidewalks and four-meter outer sidewalks leave room
    // for later planting without occupying the entrance forecourt.
    constexpr std::array<PavingRectangle, 10> abuDhabiSidewalks{{
        {-195.0f, -68.0f, 38.0f, 43.0f},
        {-195.0f, -68.0f, 170.0f, 175.0f},
        {-195.0f, -190.0f, 43.0f, 170.0f},
        {-73.0f, -68.0f, 43.0f, 50.0f},
        {-73.0f, -68.0f, 60.0f, 170.0f},
        {-209.0f, -205.0f, 28.0f, 185.0f},
        {-58.0f, -54.0f, 28.0f, 50.0f},
        {-58.0f, -54.0f, 60.0f, 185.0f},
        {-205.0f, -58.0f, 24.0f, 28.0f},
        {-205.0f, -58.0f, 185.0f, 189.0f}
    }};

    // Curbs sit inside the asphalt edges. The two east curbs open at the
    // marked pedestrian crossing, and no curb runs across a vehicle junction.
    constexpr std::array<PavingRectangle, 11> abuDhabiRoadCurbs{{
        // A 6-meter curb opening at z = 162..168 serves the side parking court.
        {-205.0f, -204.7f, 38.0f, 162.0f},
        {-205.0f, -204.7f, 168.0f, 175.0f},
        {-195.3f, -195.0f, 43.0f, 170.0f},
        {-68.0f, -67.7f, 43.0f, 50.0f},
        {-68.0f, -67.7f, 60.0f, 170.0f},
        {-58.3f, -58.0f, 38.0f, 50.0f},
        {-58.3f, -58.0f, 60.0f, 175.0f},
        {-195.0f, -68.0f, 37.7f, 38.0f},
        {-205.0f, -58.0f, 28.0f, 28.3f},
        {-195.0f, -68.0f, 175.0f, 175.3f},
        {-205.0f, -58.0f, 184.7f, 185.0f}
    }};

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
    nurAlemFacadeLocation = shader.uniformLocation("nurAlemFacade");
    abuDhabiFacadeLocation = shader.uniformLocation("abuDhabiFacade");
    abuDhabiSecondaryFacadeLocation =
        shader.uniformLocation("abuDhabiSecondaryFacadeMode");
    abuDhabiGlassMaterialLocation =
        shader.uniformLocation("abuDhabiGlassMaterial");
    talanFacadeLocation = shader.uniformLocation("talanFacadeMode");
    useGrassLocation = shader.uniformLocation("useGrass");
    useAsphaltLocation = shader.uniformLocation("useAsphalt");
    akOrdaStoneLocation = shader.uniformLocation("akOrdaStone");
    akOrdaRoofLocation = shader.uniformLocation("akOrdaRoof");
    akOrdaWaterLocation = shader.uniformLocation("akOrdaWater");
    akOrdaWaterJetLocation = shader.uniformLocation("akOrdaWaterJet");
    akOrdaWaterCenterLocation = shader.uniformLocation("akOrdaWaterCenter");
    botanicalLakeLocation = shader.uniformLocation("botanicalLake");
    greenhouseGlassLocation = shader.uniformLocation("greenhouseGlass");
    expoCanopyGlassLocation = shader.uniformLocation("expoCanopyGlass");
    sceneTimeLocation = shader.uniformLocation("sceneTime");
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

void Scene::render(const Mat4& view, const Mat4& projection, float time) const
{
    const float plazaColor[3] = {0.80f, 0.80f, 0.78f};
    const float walkwayColor[3] = {0.52f, 0.53f, 0.54f};

    shader.use();
    glUniform1i(nurAlemFacadeLocation, GL_FALSE);
    glUniform1i(goldenPanelsLocation, GL_FALSE);
    glUniform1i(shaftGlassLocation, GL_FALSE);
    glUniform1i(abuDhabiFacadeLocation, GL_FALSE);
    glUniform1i(abuDhabiSecondaryFacadeLocation, 0);
    glUniform1i(abuDhabiGlassMaterialLocation, GL_FALSE);
    glUniform1i(talanFacadeLocation, 0);
    glUniform1i(useGrassLocation, GL_FALSE);
    glUniform1i(useAsphaltLocation, GL_FALSE);
    glUniform1i(akOrdaStoneLocation, GL_FALSE);
    glUniform1i(akOrdaRoofLocation, GL_FALSE);
    glUniform1i(akOrdaWaterLocation, GL_FALSE);
    glUniform1i(akOrdaWaterJetLocation, GL_FALSE);
    glUniform1i(botanicalLakeLocation, GL_FALSE);
    glUniform1i(greenhouseGlassLocation, GL_FALSE);
    glUniform1i(expoCanopyGlassLocation, GL_FALSE);
    glUniform1f(sceneTimeLocation, time);
    glUniform1i(useTextureLocation, GL_FALSE);
    glUniform3f(lightDirectionLocation, lightDirection.x, lightDirection.y, lightDirection.z);
    glUniform1f(ambientStrengthLocation, ambientStrength);
    glUniform1f(diffuseStrengthLocation, diffuseStrength);
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, view.data());
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, projection.data());

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, groundModel.data());
    glUniform1i(useObjectColorLocation, GL_FALSE);
    mesh.drawGround();

    expoDistrict.render(modelLocation, objectColorLocation, useObjectColorLocation,
                        useGrassLocation, useTextureLocation, textureRepeatLocation, plazaTexture, shaftGlassLocation, nurAlemFacadeLocation);

    glUniform1i(useObjectColorLocation, GL_TRUE);
    botanicalGarden.renderRoads(modelLocation, objectColorLocation,
                               useTextureLocation, useGrassLocation,
                               useAsphaltLocation, textureRepeatLocation, plazaTexture);
    botanicalGarden.render(modelLocation, objectColorLocation,
                           useObjectColorLocation, useGrassLocation,
                           useTextureLocation, textureRepeatLocation,
                           botanicalLakeLocation, plazaTexture);


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

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, circularPlazaModel.data());
    glUniform3f(objectColorLocation, 0.85f, 0.84f, 0.81f);
    glUniform2f(textureRepeatLocation, 4.0f, 4.0f);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    mesh.drawCircularPlaza();

    glUniform1i(useTextureLocation, GL_FALSE);

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, circularBorderModel.data());
    glUniform3f(objectColorLocation, 0.73f, 0.74f, 0.72f);
    glPolygonOffset(-1.0f, -2.0f);
    mesh.drawCircularBorder();
    glDisable(GL_POLYGON_OFFSET_FILL);

    glBindTexture(GL_TEXTURE_2D, 0);

    // A single tapered stone court joins the processional route to Ak Orda's
    // entrance stair. Its outer edge descends to the surrounding terrain.
    constexpr Vec3 palace = akOrdaPosition;
    Mat4 ceremonialCourt = modelMatrix(
        {palace.x, 0.07f, palace.z}, 0, {1, 1, 1}
    );

    glBindTexture(GL_TEXTURE_2D, plazaTexture);
    glUniform1i(useTextureLocation, GL_TRUE);
    glUniform3f(objectColorLocation, 0.87f, 0.86f, 0.83f);
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, ceremonialCourt.data());
    glUniform2f(textureRepeatLocation, 1.0f, 1.0f);
    mesh.drawCeremonialCourt();
    glUniform1i(useTextureLocation, GL_FALSE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Flat stone inlays mark a broad clear central axis through the court.
    glUniform3f(objectColorLocation, 0.77f, 0.76f, 0.73f);
    for (float side : {-1.0f, 1.0f})
    {
        Mat4 inlay = modelMatrix(
            {palace.x + side * 17.0f, 0.077f, palace.z - 99.0f},
            0, {0.35f, 0.025f, 102.0f}
        );
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, inlay.data());
        mesh.drawCube();
    }

    // Long matched parterres integrate the lawns and fountains into the same
    // paved forecourt, leaving their green interiors for later flower patterns.
    for (float side : {-1.0f, 1.0f})
    {
        float x = palace.x + side * 36.0f;
        Mat4 stoneBed = modelMatrix(
            {x, 0.085f, palace.z - 96.0f}, 0, {14.0f, 1.0f, 15.0f}
        );
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, stoneBed.data());
        glUniform3f(objectColorLocation, 0.77f, 0.76f, 0.72f);
        mesh.drawUnitCapsule();

        Mat4 lawn = modelMatrix(
            {x, 0.105f, palace.z - 111.0f}, 0, {11.0f, 1.0f, 9.0f}
        );
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, lawn.data());
        glUniform3f(objectColorLocation, 0.34f, 0.47f, 0.32f);
        glUniform1i(useGrassLocation, GL_TRUE);
        mesh.drawUnitCapsule();
        glUniform1i(useGrassLocation, GL_FALSE);
    }

    constexpr std::array<Vec3, 4> parterreColors{{
        {0.72f, 0.25f, 0.27f}, // Red border blossoms.
        {0.88f, 0.71f, 0.28f}, // Yellow diamond outlines.
        {0.91f, 0.90f, 0.84f}, // Ivory-white inner borders.
        {0.57f, 0.41f, 0.62f}  // Restrained purple centers.
    }};

    for (float side : {-1.0f, 1.0f})
    {
        // Both beds reuse the same local pattern, which is symmetric about
        // its own centerline and therefore mirrors across the palace axis.
        Mat4 pattern = modelMatrix(
            {palace.x + side * 36.0f, 0.105f, palace.z - 111.0f},
            0, {1, 1, 1}
        );
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, pattern.data());
        glUniform3f(objectColorLocation, 0.19f, 0.32f, 0.17f);
        mesh.drawParterreStems();

        for (std::size_t color = 0; color < parterreColors.size(); ++color)
        {
            const Vec3& petals = parterreColors[color];
            glUniform3f(objectColorLocation, petals.x, petals.y, petals.z);
            mesh.drawParterreFlowers(static_cast<int>(color));
        }
    }

    // Narrow planted strips run beside the central stone inlays, leaving a
    // 34-meter unobstructed route between them.
    constexpr std::array<Vec3, 2> approachFlowerColors{{
        {0.84f, 0.70f, 0.30f}, {0.58f, 0.43f, 0.63f}
    }};
    constexpr std::array<float, 6> shrubZ{{238, 248, 258, 268, 278, 288}};

    for (float side : {-1.0f, 1.0f})
    {
        float x = palace.x + side * 19.5f;
        float z = palace.z - 108.0f;
        Mat4 stoneStrip = modelMatrix({x, 0.085f, z}, 0,
                                       {1.6f, 1.0f, 11.0f});
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, stoneStrip.data());
        glUniform3f(objectColorLocation, 0.77f, 0.76f, 0.72f);
        mesh.drawUnitCapsule();

        Mat4 soilStrip = modelMatrix({x, 0.101f, z}, 0,
                                      {1.3f, 1.0f, 10.5f});
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, soilStrip.data());
        glUniform3f(objectColorLocation, 0.28f, 0.24f, 0.19f);
        mesh.drawUnitCapsule();

        Mat4 flowers = modelMatrix({x, 0.101f, z}, 0, {1, 1, 1});
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, flowers.data());
        glUniform3f(objectColorLocation, 0.19f, 0.32f, 0.17f);
        mesh.drawApproachFlowerStems();

        for (std::size_t color = 0; color < approachFlowerColors.size(); ++color)
        {
            const Vec3& petals = approachFlowerColors[color];
            glUniform3f(objectColorLocation, petals.x, petals.y, petals.z);
            mesh.drawApproachFlowers(static_cast<int>(color));
        }

        // Low border shrubs sit just beyond each flower strip.
        glUniform3f(objectColorLocation, 0.28f, 0.43f, 0.27f);
        for (float shrubPositionZ : shrubZ)
        {
            Mat4 shrub = modelMatrix(
                {palace.x + side * 21.8f, 0.37f, shrubPositionZ},
                0, {0.42f, 0.28f, 0.50f}
            );
            glUniformMatrix4fv(modelLocation, 1, GL_FALSE, shrub.data());
            mesh.drawSphere();
        }
    }

    // A flush six-petal stone medallion marks the axis near the entrance.
    constexpr float accentZ = akOrdaPosition.z - 52.0f;
    Mat4 accentBase = modelMatrix({palace.x, 0.078f, accentZ}, 0,
                                  {4.5f, 1.0f, 4.5f});
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, accentBase.data());
    glUniform3f(objectColorLocation, 0.85f, 0.83f, 0.78f);
    mesh.drawUnitDisk();

    Mat4 accentRim = modelMatrix({palace.x, 0.080f, accentZ}, 0,
                                 {4.5f, 1.0f, 4.5f});
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, accentRim.data());
    glUniform3f(objectColorLocation, 0.71f, 0.69f, 0.64f);
    mesh.drawUnitRing();

    for (int petal = 0; petal < 6; ++petal)
    {
        float angle = 2.0f * pi * petal / 6.0f;
        Mat4 petalModel = modelMatrix(
            {palace.x + 2.4f * std::cos(angle), 0.090f,
             accentZ + 2.4f * std::sin(angle)},
            90.0f - angle * 180.0f / pi, {0.95f, 1.0f, 1.55f}
        );
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, petalModel.data());
        if (petal % 2 == 0)
        {
            glUniform3f(objectColorLocation, 0.67f, 0.34f, 0.38f);
        }
        else
        {
            glUniform3f(objectColorLocation, 0.77f, 0.73f, 0.65f);
        }
        mesh.drawUnitDisk();
    }

    Mat4 accentCenter = modelMatrix({palace.x, 0.100f, accentZ}, 0,
                                    {0.95f, 1.0f, 0.95f});
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, accentCenter.data());
    glUniform3f(objectColorLocation, 0.85f, 0.69f, 0.33f);
    mesh.drawUnitDisk();

    // Each fountain completes the palace end of one elongated parterre.
    for (float side : {-1.0f, 1.0f})
    {
        float x = palace.x + side * 36.0f;
        float z = palace.z - 67.0f;
        Mat4 basinWall = modelMatrix({x, 0.085f, z}, 0,
                                     {10.5f, 0.325f, 13.0f});
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, basinWall.data());
        glUniform3f(objectColorLocation, 0.80f, 0.79f, 0.75f);
        mesh.drawUnitCylinderWall();

        Mat4 basinRim = modelMatrix({x, 0.41f, z}, 0,
                                    {10.5f, 1.0f, 13.0f});
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, basinRim.data());
        glUniform3f(objectColorLocation, 0.92f, 0.91f, 0.87f);
        mesh.drawUnitRing();

        Mat4 water = modelMatrix({x, 0.29f, z}, 0,
                                 {10.5f, 1.0f, 13.0f});
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, water.data());
        glUniform3f(objectColorLocation, 0.49f, 0.70f, 0.78f);
        glUniform2f(akOrdaWaterCenterLocation, x, z);
        glUniform1i(akOrdaWaterLocation, GL_TRUE);
        mesh.drawUnitDisk();
        glUniform1i(akOrdaWaterLocation, GL_FALSE);

        Mat4 streams = modelMatrix({x, 0.29f, z}, 0, {1, 1, 1});
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, streams.data());
        glUniform3f(objectColorLocation, 0.79f, 0.91f, 0.97f);
        glUniform1i(akOrdaWaterJetLocation, GL_TRUE);
        mesh.drawFountainJets();
        glUniform1i(akOrdaWaterJetLocation, GL_FALSE);
    }

    // Four rotations keep the lawns and their small soil beds symmetric.
    glEnable(GL_POLYGON_OFFSET_FILL);

    for (int garden = 0; garden < 4; ++garden)
    {
        float rotation = static_cast<float>(garden) * 90.0f;
        Mat4 lawnModel = modelMatrix({0, 0.10f, 0}, rotation, {1, 1, 1});
        Mat4 innerBandModel = modelMatrix({0, 0.115f, 0}, rotation, {1, 1, 1});
        Mat4 flowerbedModel = modelMatrix({0, 0.14f, 0}, rotation, {1, 1, 1});

        glPolygonOffset(-1.0f, -3.0f);
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, lawnModel.data());
        glUniform3f(objectColorLocation, 0.32f, 0.45f, 0.30f);
        glUniform1i(useGrassLocation, GL_TRUE);
        mesh.drawGardenLawn();

        glPolygonOffset(-1.0f, -4.0f);
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, innerBandModel.data());
        glUniform3f(objectColorLocation, 0.36f, 0.49f, 0.33f);
        mesh.drawGardenInnerBand();
        glUniform1i(useGrassLocation, GL_FALSE);

        glPolygonOffset(-1.0f, -5.0f);
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, flowerbedModel.data());
        glUniform3f(objectColorLocation, 0.25f, 0.18f, 0.12f);
        mesh.drawGardenSoil();

        glUniform3f(objectColorLocation, 0.70f, 0.69f, 0.65f);
        mesh.drawGardenBedBorder();
    }

    glDisable(GL_POLYGON_OFFSET_FILL);

    // Small stems, paired leaves and faceted blossoms repeat within each bed.
    constexpr float blossomColors[3][3] = {
        {0.68f, 0.27f, 0.27f},
        {0.76f, 0.61f, 0.29f},
        {0.76f, 0.44f, 0.25f}
    };

    for (int garden = 0; garden < 4; ++garden)
    {
        float rotation = static_cast<float>(garden) * 90.0f;

        for (int row = 0; row < 2; ++row)
        {
            float radius = row == 0 ? 29.5f : 30.5f;

            for (int column = 0; column < 6; ++column)
            {
                float angle = (39.0f + column * 2.6f - rotation) * pi / 180.0f;
                float x = radius * std::cos(angle);
                float z = radius * std::sin(angle);

                Mat4 stem = modelMatrix({x, 0.36f, z}, 0, {0.045f, 0.44f, 0.045f});
                glUniform3f(objectColorLocation, 0.18f, 0.33f, 0.16f);
                glUniformMatrix4fv(modelLocation, 1, GL_FALSE, stem.data());
                mesh.drawCube();

                glUniform3f(objectColorLocation, 0.26f, 0.43f, 0.20f);
                Mat4 leftLeaf = modelMatrix({x - 0.14f, 0.35f, z}, 0, {0.22f, 0.05f, 0.10f});
                glUniformMatrix4fv(modelLocation, 1, GL_FALSE, leftLeaf.data());
                mesh.drawCube();

                Mat4 rightLeaf = modelMatrix({x + 0.14f, 0.44f, z}, 0, {0.22f, 0.05f, 0.10f});
                glUniformMatrix4fv(modelLocation, 1, GL_FALSE, rightLeaf.data());
                mesh.drawCube();

                int colorIndex = (row + column) % 3;
                float variation = 0.94f + 0.03f * ((row + 2 * column) % 3);
                const float* color = blossomColors[colorIndex];
                glUniform3f(
                    objectColorLocation,
                    color[0] * variation, color[1] * variation, color[2] * variation
                );
                Mat4 blossom = modelMatrix({x, 0.55f, z}, 0, {0.52f, 0.50f, 0.52f});
                glUniformMatrix4fv(modelLocation, 1, GL_FALSE, blossom.data());
                mesh.drawFlowerBlossom();
            }
        }
    }

    // Warm, light paving distinguishes the gallery forecourt from the boulevard.
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, talanForecourtModel.data());
    glUniform3f(objectColorLocation, 0.78f, 0.77f, 0.74f);
    glBindTexture(GL_TEXTURE_2D, plazaTexture);
    glUniform2f(textureRepeatLocation, 60.0f / 12.0f, 27.0f / 12.0f);
    glUniform1i(useTextureLocation, GL_TRUE);
    mesh.drawCube();

    // Light stone slabs form Abu Dhabi Plaza's forecourt and two perpendicular
    // pedestrian segments. Every rectangle meets its neighbor at one edge.
    glUniform3f(objectColorLocation, 0.80f, 0.80f, 0.79f);

    for (const PavingRectangle& area : abuDhabiPaving)
    {
        float width = area.maxX - area.minX;
        float length = area.maxZ - area.minZ;
        Mat4 paving = modelMatrix(
            {(area.minX + area.maxX) * 0.5f, 0,
             (area.minZ + area.maxZ) * 0.5f},
            0, {width, 0.1f, length}
        );
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, paving.data());
        glUniform2f(textureRepeatLocation, width / 12.0f, length / 12.0f);
        mesh.drawCube();
    }

    // Four 18.75 cm risers bring the route up to the existing door sill at
    // y = 0.8 without changing the approved podium or entrance geometry.
    for (int step = 0; step < 4; ++step)
    {
        float top = 0.05f + (step + 1) * 0.1875f;
        Mat4 stair = modelMatrix(
            {-123.0f, top * 0.5f, 83.5f + step},
            0, {24.0f, top, 1.0f}
        );
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, stair.data());
        glUniform2f(textureRepeatLocation, 2.0f, 1.0f / 12.0f);
        mesh.drawCube();
    }

    Mat4 entranceLanding = modelMatrix(
        {-123.0f, 0.4f, 89.5f}, 0, {24.0f, 0.8f, 5.0f}
    );
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, entranceLanding.data());
    glUniform2f(textureRepeatLocation, 2.0f, 5.0f / 12.0f);
    mesh.drawCube();

    for (const PavingRectangle& area : abuDhabiRaisedBase)
    {
        float width = area.maxX - area.minX;
        float length = area.maxZ - area.minZ;
        Mat4 base = modelMatrix(
            {(area.minX + area.maxX) * 0.5f, 0.4f,
             (area.minZ + area.maxZ) * 0.5f},
            0, {width, 0.8f, length}
        );
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, base.data());
        glUniform2f(textureRepeatLocation, width / 12.0f, length / 12.0f);
        mesh.drawCube();
    }

    glUniform1i(useTextureLocation, GL_FALSE);
    glUniform3f(objectColorLocation, 0.67f, 0.69f, 0.69f);

    for (const PavingRectangle& edge : abuDhabiBorders)
    {
        Mat4 border = modelMatrix(
            {(edge.minX + edge.maxX) * 0.5f, 0.06f,
             (edge.minZ + edge.maxZ) * 0.5f},
            0, {edge.maxX - edge.minX, 0.12f, edge.maxZ - edge.minZ}
        );
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, border.data());
        mesh.drawCube();
    }

    auto drawAbuDhabiRectangle = [&](const PavingRectangle& area, float top)
    {
        float width = area.maxX - area.minX;
        float length = area.maxZ - area.minZ;
        Mat4 model = modelMatrix(
            {(area.minX + area.maxX) * 0.5f, top * 0.5f,
             (area.minZ + area.maxZ) * 0.5f},
            0, {width, top, length}
        );
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model.data());
        mesh.drawCube();
    };

    // A compact connected block has two 10-meter east-west streets and two
    // north-south streets. The crossing covers only the approved route's
    // 10-meter intersection with the east street.
    glUniform3f(objectColorLocation, 0.27f, 0.29f, 0.30f);
    glUniform1i(useAsphaltLocation, GL_TRUE);

    for (const PavingRectangle& road : abuDhabiRoads)
    {
        drawAbuDhabiRectangle(road, 0.04f);
    }

    drawAbuDhabiRectangle(abuDhabiCrossing, 0.065f);
    glUniform1i(useAsphaltLocation, GL_FALSE);

    // Short centerline dashes identify two-way vehicle lanes without drawing
    // through the junctions or the marked pedestrian crossing.
    glUniform3f(objectColorLocation, 0.78f, 0.79f, 0.75f);

    for (float centerZ : {33.0f, 180.0f})
    {
        for (float x = -191.0f; x < -72.0f; x += 14.0f)
        {
            drawAbuDhabiRectangle({x, x + 6.0f, centerZ - 0.075f,
                                    centerZ + 0.075f}, 0.046f);
        }
    }

    for (float centerX : {-200.0f, -63.0f})
    {
        for (float z = 45.0f; z < 165.0f; z += 14.0f)
        {
            if (centerX == -63.0f && z < 61.0f)
            {
                continue;
            }

            drawAbuDhabiRectangle({centerX - 0.075f, centerX + 0.075f,
                                    z, z + 6.0f}, 0.046f);
        }
    }

    // Zebra bars cross the entire east street while leaving dark asphalt
    // between them, so both vehicle and pedestrian routes remain legible.
    glUniform3f(objectColorLocation, 0.88f, 0.89f, 0.86f);

    for (int stripe = 0; stripe < 6; ++stripe)
    {
        float z = 50.45f + stripe * 1.5f;
        drawAbuDhabiRectangle({-67.55f, -58.45f, z, z + 0.7f}, 0.072f);
    }

    // Light stone sidewalks meet at the block corners. The east-side pair
    // leaves the established route open through the marked crossing.
    glUniform3f(objectColorLocation, 0.78f, 0.79f, 0.77f);
    glBindTexture(GL_TEXTURE_2D, plazaTexture);
    glUniform1i(useTextureLocation, GL_TRUE);

    for (const PavingRectangle& sidewalk : abuDhabiSidewalks)
    {
        float width = sidewalk.maxX - sidewalk.minX;
        float length = sidewalk.maxZ - sidewalk.minZ;
        glUniform2f(textureRepeatLocation, width / 12.0f, length / 12.0f);
        drawAbuDhabiRectangle(sidewalk, 0.06f);
    }

    glUniform1i(useTextureLocation, GL_FALSE);
    glUniform3f(objectColorLocation, 0.74f, 0.75f, 0.73f);

    for (const PavingRectangle& curb : abuDhabiRoadCurbs)
    {
        drawAbuDhabiRectangle(curb, 0.12f);
    }

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, walkwayModel.data());
    glUniform3fv(objectColorLocation, 1, walkwayColor);
    glBindTexture(GL_TEXTURE_2D, walkwayTexture);
    glUniform2f(textureRepeatLocation, 1.5f, 7.5f);
    glUniform1i(useTextureLocation, GL_TRUE);
    mesh.drawCube();

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, oppositeWalkwayModel.data());
    mesh.drawCube();

    // Meet the negative-Z path at x = 6 and the forecourt at x = 55.
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, talanApproachModel.data());
    glUniform2f(textureRepeatLocation, 49.0f / 8.0f, 1.0f);
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
    talanTowers.render(mesh, modelLocation, objectColorLocation, talanFacadeLocation);
    glUniform1i(talanFacadeLocation, 0);
    abuDhabiPlaza.render(
        mesh, modelLocation, objectColorLocation,
        shaftGlassLocation, abuDhabiFacadeLocation,
        abuDhabiSecondaryFacadeLocation, abuDhabiGlassMaterialLocation
    );
    glUniform1i(useTextureLocation, GL_FALSE);
    abuDhabiLandscaping.render(
        mesh, modelLocation, objectColorLocation, useGrassLocation
    );
    abuDhabiFurniture.render(
        mesh, modelLocation, objectColorLocation, useAsphaltLocation
    );

    glUniform1i(useTextureLocation, GL_FALSE);
    akOrda.render(mesh, modelLocation, objectColorLocation,
                  akOrdaStoneLocation, akOrdaRoofLocation);

    for (const Tree& tree : trees)
    {
        tree.render(mesh, modelLocation, objectColorLocation);
    }

    for (const StreetLight& streetLight : streetLights)
    {
        streetLight.render(mesh, modelLocation, objectColorLocation);
    }

    for (const StreetLight& streetLight : akOrdaApproachLights)
    {
        streetLight.render(mesh, modelLocation, objectColorLocation);
    }

    for (const Bench& bench : benches)
    {
        bench.render(mesh, modelLocation, objectColorLocation);
    }
    // These glass volumes occupy distant, disjoint districts. Order the two
    // batches back-to-front, with per-triangle sorting inside each volume.
    // Both follow all opaque scenery and restore blending/depth-write state.
    auto depth = [&](Vec3 p) { return view[2]*p.x + view[6]*p.y + view[10]*p.z; };
    auto greenhouse = [&] {
        botanicalGarden.renderGreenhouseGlass(modelLocation, objectColorLocation,
                                               greenhouseGlassLocation, view);
    };
    auto canopy = [&] {
        expoDistrict.renderCanopyGlass(modelLocation, objectColorLocation,
                                       expoCanopyGlassLocation, view);
    };
    if (depth(ExpoDistrict::center) < depth(BotanicalGarden::center)) { canopy(); greenhouse(); }
    else { greenhouse(); canopy(); }
}
