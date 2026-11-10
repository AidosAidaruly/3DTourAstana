#pragma once

#include "Math3D.h"

#include <glad/glad.h>

#include <cstddef>
#include <vector>

// Ground-level garden layout, planting zones, paths, and ornamental lake.
class BotanicalGarden
{
public:
    static constexpr Vec3 center{-370.0f, 0.0f, -130.0f};

    BotanicalGarden();
    ~BotanicalGarden();

    BotanicalGarden(const BotanicalGarden&) = delete;
    BotanicalGarden& operator=(const BotanicalGarden&) = delete;

    void render(GLint modelLocation, GLint objectColorLocation,
                GLint useObjectColorLocation, GLint useGrassLocation,
                GLint useTextureLocation, GLint textureRepeatLocation,
                GLint botanicalLakeLocation, GLuint stoneTexture) const;
    void renderGreenhouseGlass(GLint modelLocation, GLint objectColorLocation,
                               GLint glassLocation, const Mat4& view) const;
    void renderRoads(GLint modelLocation, GLint objectColorLocation,
                     GLint useTextureLocation, GLint useGrassLocation,
                     GLint useAsphaltLocation, GLint textureRepeatLocation,
                     GLuint stoneTexture) const;

private:
    struct IndexRange
    {
        std::size_t firstIndex = 0;
        GLsizei count = 0;
    };

    void drawRange(const IndexRange& range) const;

    Mat4 model;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint glassEbo = 0;
    GLuint entranceSignTexture = 0;
    struct GlassTriangle
    {
        std::array<GLuint, 3> indices;
        Vec3 worldCenter;
    };
    std::vector<GlassTriangle> glassTriangles;
    IndexRange district;
    IndexRange border;
    IndexRange lightSectors;
    IndexRange darkSectors;
    IndexRange flowerSoil;
    IndexRange flowerRed;
    IndexRange flowerGold;
    IndexRange flowerPurple;
    IndexRange radialPaths;
    IndexRange outwardPaths;
    IndexRange mainAxis;
    IndexRange innerCurvedPath;
    IndexRange middleCurvedPath;
    IndexRange ornamentalPaths;
    IndexRange circularPath;
    IndexRange centralPlaza;
    IndexRange lakeGrassBank;
    IndexRange lakeSoilBank;
    IndexRange lakeShorePath;
    IndexRange lakeWater;
    IndexRange bridgeDecks;
    IndexRange bridgeRails;
    IndexRange lakesideBenchFrames;
    IndexRange lakesideWood;
    IndexRange lakesideDiffusers;
    IndexRange treeTrunks;
    IndexRange clusteredFoliage;
    IndexRange columnarFoliage;
    IndexRange coniferFoliage;
    IndexRange hedgesAndLeaves;
    IndexRange blossoms[4];
    IndexRange greenhouseStone;
    IndexRange greenhouseFrame;
    IndexRange greenhousePlants;
    IndexRange greenhouseFreshFoliage;
    IndexRange greenhouseStems;
    IndexRange greenhouseSoil;
    IndexRange greenhouseGlass;
    IndexRange entrancePaving;
    IndexRange entranceStone;
    IndexRange entranceTrim;
    IndexRange entranceSigns;
    IndexRange entranceSoil;
    IndexRange entranceLeaves;
    IndexRange entranceFlowers[2];
    IndexRange colonnadeCourt;
    IndexRange colonnadeStone;
    IndexRange colonnadeTrim;
    IndexRange perimeterAsphalt;
    IndexRange perimeterSidewalks;
    IndexRange perimeterCurbs;
    IndexRange perimeterMarkings;
    IndexRange perimeterLawn;
    IndexRange perimeterTreeTrunks;
    IndexRange perimeterTreeFoliage;
    IndexRange perimeterLightFrames;
    IndexRange perimeterLightDiffusers;
};
