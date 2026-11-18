#pragma once

#include "Math3D.h"
#include "AstanaArenaSeats.h"
#include <glad/glad.h>
#include <array>
#include <cstddef>

// Static oval shell with opaque aluminum and reflective facade glazing.
class SunShadows;

class AstanaArena
{
public:
    static constexpr Vec3 center{-370.0f, 0.0f, -435.0f};
    static constexpr float length = 240.0f, width = 196.0f;
    static constexpr float foundationLength = 248.0f, foundationWidth = 200.0f;
    static constexpr float retractableLength = 140.0f, retractableWidth = 88.0f;
    static constexpr float roofHeight = 39.746357f;
    static constexpr float pitchLength = 105.0f, pitchWidth = 68.0f, pitchY = .60f;
    static Vec3 seatingContour(float angle,float t);

    AstanaArena();
    ~AstanaArena();
    AstanaArena(const AstanaArena&) = delete;
    AstanaArena& operator=(const AstanaArena&) = delete;
    void render(GLint modelLocation, GLint colorLocation, GLint objectLocation,
                GLint facadeLocation, GLint asphaltLocation) const;
    void renderSeats(const Mat4& view,const Mat4& projection,Vec3 light) const
    {seating.render(view,projection,light);}
    AstanaArenaSeats::Statistics seatingStatistics() const {return seating.statistics();}

    void renderShadow(SunShadows& shadows)const;

private:
    struct Range { std::size_t first = 0; GLsizei count = 0; };
    std::array<Range, 30> ranges{};
    GLuint vao = 0, vbo = 0, ebo = 0;
    AstanaArenaSeats seating;
};
