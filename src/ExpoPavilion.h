#pragma once

#include "Math3D.h"
#include <glad/glad.h>
#include <cstddef>

// Independently parameterized cubic-Bezier petal and open courtyard, around Nur Alem.
class ExpoPavilion
{
public:
    explicit ExpoPavilion(Vec3 districtCenter, int quadrant);
    ~ExpoPavilion();
    ExpoPavilion(const ExpoPavilion&) = delete;
    ExpoPavilion& operator=(const ExpoPavilion&) = delete;
    void render(GLint modelLocation, GLint colorLocation, GLint glassLocation) const;

private:
    struct Range { std::size_t first = 0; GLsizei count = 0; };
    void draw(const Range& range) const;
    Mat4 model;
    GLuint vao = 0, vbo = 0, ebo = 0;
    Range roof, roofAlternate, roofGlazing, structure, soffit, trim, details, glass, glassLight, glassDeep;
};
