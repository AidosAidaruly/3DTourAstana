#pragma once

#include "Baiterek.h"
#include "Building.h"
#include "Math3D.h"
#include "Mesh.h"
#include "Shader.h"

#include <filesystem>

class Scene
{
public:
    explicit Scene(const std::filesystem::path& shaderDirectory);
    void render(const Mat4& view, const Mat4& projection) const;

private:
    Shader shader;
    Mesh mesh;
    Baiterek baiterek;

    // One 16 x 25 x 12 building, resting on the ground outside the plaza.
    Building building{{75, 12.5f, 0}, {16, 25, 12}, {0.36f, 0.46f, 0.56f}};
    Building secondBuilding{{100, 17.5f, -30}, {20, 35, 14}, {0.62f, 0.69f, 0.76f}};

    GLint viewLocation = -1;
    GLint projectionLocation = -1;
    GLint modelLocation = -1;
    GLint useObjectColorLocation = -1;
    GLint objectColorLocation = -1;

    // World scale: 1 unit = 1 meter; Baiterek's reference point stays at the origin.
    // Scale the existing 100 x 100 ground mesh to 400 x 400 meters.
    Mat4 groundModel = modelMatrix({0, 0, 0}, 0, {4, 1, 4});

    // The 100 x 60 meter plaza is centered at the origin; its top is at y = 0.05.
    Mat4 plazaModel = modelMatrix({0, 0, 0}, 0, {100, 0.1f, 60});

    // Meet the plaza at z = 30 without overlapping its top surface.
    // The 12 x 60 meter walkway has the same top height: y = 0.05.
    Mat4 walkwayModel = modelMatrix({0, 0, 60}, 0, {12, 0.1f, 60});

    // Match the existing walkway and meet the opposite plaza edge at z = -30.
    Mat4 oppositeWalkwayModel = modelMatrix({0, 0, -60}, 0, {12, 0.1f, 60});
};
