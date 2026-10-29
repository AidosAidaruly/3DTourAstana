#pragma once

#include <array>

constexpr float pi = 3.14159265358979323846f;

struct Vec3
{
    float x, y, z;
};

// Matrices use column-major storage, as expected by OpenGL.
using Mat4 = std::array<float, 16>;

Vec3 operator+(Vec3 a, Vec3 b);
Vec3 operator-(Vec3 a, Vec3 b);
Vec3 operator*(Vec3 a, float s);
float dot(Vec3 a, Vec3 b);
Vec3 cross(Vec3 a, Vec3 b);
Vec3 normalize(Vec3 v);

// Scale locally, rotate around Y, then translate: T * Ry * S.
Mat4 modelMatrix(Vec3 translation, float rotationDegrees, Vec3 scale);
// Position a component relative to a parent that rotates around the vertical axis.
Mat4 partModelMatrix(Vec3 origin, float rotationDegrees, Vec3 localCenter, Vec3 scale);
Mat4 projectionMatrix(float aspect);
