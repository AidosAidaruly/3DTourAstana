#pragma once

#include "Math3D.h"

#include <glad/glad.h>

#include <array>
#include <cstddef>

// Ground-level landscape and road around Khan Shatyr, with six tower sites.
class KhanDistrict
{
public:
    struct TowerSite
    {
        Vec3 position;
        float width;
        float depth;
        float height;
    };

    // The shared site data keeps towers, forecourts, and radial paths aligned.
    static constexpr std::array<TowerSite, 6> towerSites{{
        {{-78, 0, -342}, 14.0f, 16.0f, 42.0f},
        {{78, 0, -342}, 14.0f, 16.0f, 44.0f},
        {{-100, 0, -466}, 16.0f, 18.0f, 52.0f},
        {{100, 0, -466}, 16.0f, 18.0f, 55.0f},
        {{-46, 0, -520}, 14.0f, 16.0f, 38.0f},
        {{46, 0, -520}, 14.0f, 16.0f, 40.0f}
    }};

    explicit KhanDistrict(Vec3 position);
    ~KhanDistrict();

    KhanDistrict(const KhanDistrict&) = delete;
    KhanDistrict& operator=(const KhanDistrict&) = delete;

    void render(GLint modelLocation, GLint objectColorLocation,
                GLint useGrassLocation, GLint useTextureLocation) const;

private:
    struct IndexRange
    {
        std::size_t firstIndex = 0;
        GLsizei count = 0;
    };

    void drawRange(const IndexRange& range, Vec3 color, GLint objectColorLocation) const;

    Mat4 model;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    IndexRange lawnRange;
    IndexRange entranceLawnRange;
    IndexRange pathRange;
    IndexRange roadRange;
    IndexRange markingRange;
    IndexRange curbRange;
    IndexRange radialPavingRange;
    IndexRange crossingRange;
    IndexRange crossingStripeRange;
    IndexRange edgeRange;
    IndexRange outerRoadRange;
    IndexRange mainCrossingRange;
    IndexRange vehicleLinkRange;
    IndexRange parkingRange;
    IndexRange vehicleMarkingRange;
    IndexRange parkingMarkingRange;
    IndexRange outerCurbRange;
    IndexRange parkingCurbRange;
};
