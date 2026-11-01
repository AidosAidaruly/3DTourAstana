#pragma once

#include "AbuDhabiFurniture.h"
#include "AbuDhabiLandscaping.h"
#include "AbuDhabiPlaza.h"
#include "Baiterek.h"
#include "Bench.h"
#include "Building.h"
#include "Math3D.h"
#include "Mesh.h"
#include "Shader.h"
#include "StreetLight.h"
#include "TalanTowers.h"
#include "Tree.h"

#include <array>
#include <filesystem>

class Scene
{
public:
    explicit Scene(const std::filesystem::path& shaderDirectory);
    ~Scene();
    void render(const Mat4& view, const Mat4& projection) const;

private:
    Shader shader;
    Mesh mesh;
    Baiterek baiterek;
    // Two rows of four trees, clear of the walkway and plaza.
    std::array<Tree, 8> trees{{
        Tree({-14, 0, 40}),
        Tree({-14, 0, 54}),
        Tree({-14, 0, 68}),
        Tree({-14, 0, 82}),
        Tree({14, 0, 40}),
        Tree({14, 0, 54}),
        Tree({14, 0, 68}),
        Tree({14, 0, 82})
    }};

    // Four lights frame the gardens; four mark the walkway entrances.
    std::array<StreetLight, 8> streetLights{{
        StreetLight({-39, 0.05f, -18}, -90),
        StreetLight({-39, 0.05f, 18}, -90),
        StreetLight({39, 0.05f, -18}, 90),
        StreetLight({39, 0.05f, 18}, 90),
        StreetLight({-9, 0, -36}, -90),
        StreetLight({9, 0, -36}, 90),
        StreetLight({-9, 0, 36}, -90),
        StreetLight({9, 0, 36}, 90)
    }};

    // Four seats border the curved lawns; two frame the camera-side approach.
    // Local bench forward is -Z, so these rotations face the monument.
    std::array<Bench, 6> benches{{
        Bench({-30, 0.05f, -19}, -122),
        Bench({-30, 0.05f, 19}, -58),
        Bench({30, 0.05f, -19}, 122),
        Bench({30, 0.05f, 19}, 58),
        Bench({-11, 0.05f, 27}, -22),
        Bench({11, 0.05f, 27}, 22)
    }};

    // One 16 x 25 x 12 building, resting on the ground outside the plaza.
    // Six floors at 3.75-meter spacing, with 1.8 x 2.1 meter windows.
    Building building{
        {75, 12.5f, 0}, {16, 25, 12}, {0.36f, 0.46f, 0.56f},
        WindowLayout{6, 4, 3, 1.25f, 0.45f, 0.56f}
    };
    Building secondBuilding{{140, 17.5f, -20}, {20, 35, 14}, {0.62f, 0.69f, 0.76f}};

    // Opposite-side buildings stay clear of the plaza and pedestrian axis.
    Building thirdBuilding{{-75, 15, 0}, {18, 30, 14}, {0.48f, 0.57f, 0.63f}};
    Building fourthBuilding{{-100, 20, -35}, {22, 40, 16}, {0.67f, 0.70f, 0.72f}};

    // The shared podium stays clear of the existing buildings and park.
    TalanTowers talanTowers{{85, 0, -65}};
    AbuDhabiPlaza abuDhabiPlaza{{-125, 0, 125}};
    AbuDhabiLandscaping abuDhabiLandscaping;
    AbuDhabiFurniture abuDhabiFurniture;

    GLint viewLocation = -1;
    GLint projectionLocation = -1;
    GLint modelLocation = -1;
    GLint useObjectColorLocation = -1;
    GLint objectColorLocation = -1;
    GLint useTextureLocation = -1;
    GLint plazaTextureLocation = -1;
    GLint textureRepeatLocation = -1;
    GLuint plazaTexture = 0;
    GLuint walkwayTexture = 0;
    GLint lightDirectionLocation = -1;
    GLint ambientStrengthLocation = -1;
    GLint diffuseStrengthLocation = -1;
    GLint goldenPanelsLocation = -1;
    GLint shaftGlassLocation = -1;
    GLint abuDhabiFacadeLocation = -1;
    GLint abuDhabiSecondaryFacadeLocation = -1;
    GLint abuDhabiGlassMaterialLocation = -1;
    GLint talanFacadeLocation = -1;
    GLint useGrassLocation = -1;
    GLint useAsphaltLocation = -1;

    // Daylight direction points from the sun toward the scene.
    Vec3 lightDirection{-0.5f, -1.0f, -0.3f};
    float ambientStrength = 0.35f;
    float diffuseStrength = 0.65f;

    // World scale: 1 unit = 1 meter; Baiterek's reference point stays at the origin.
    // Cover the Abu Dhabi Plaza roads while retaining the larger city terrain.
    Mat4 groundModel = modelMatrix({0, 0, -75}, 0, {6, 1, 10.5f});

    // The 100 x 60 meter plaza is centered at the origin; its top is at y = 0.05.
    Mat4 plazaModel = modelMatrix({0, 0, 0}, 0, {100, 0.1f, 60});

    // A low annular paved area surrounds the monument without covering its base.
    Mat4 circularPlazaModel = modelMatrix({0, 0.09f, 0}, 0, {1, 1, 1});
    Mat4 circularBorderModel = modelMatrix({0, 0.11f, 0}, 0, {1, 1, 1});

    // Meet the plaza at z = 30 without overlapping its top surface.
    // The 12 x 60 meter walkway has the same top height: y = 0.05.
    Mat4 walkwayModel = modelMatrix({0, 0, 60}, 0, {12, 0.1f, 60});

    // Match the existing walkway and meet the opposite plaza edge at z = -30.
    Mat4 oppositeWalkwayModel = modelMatrix({0, 0, -60}, 0, {12, 0.1f, 60});

    // The gallery has its own paved forecourt and a branch from the city path.
    Mat4 talanForecourtModel = modelMatrix({85, 0, -41.5f}, 0, {60, 0.1f, 27});
    Mat4 talanApproachModel = modelMatrix({30.5f, 0, -50}, 0, {49, 0.1f, 8});
};
