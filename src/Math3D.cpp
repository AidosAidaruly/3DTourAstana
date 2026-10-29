#include "Math3D.h"

#include <cmath>

Vec3 operator+(Vec3 a, Vec3 b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3 operator-(Vec3 a, Vec3 b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 operator*(Vec3 a, float s)
{
    return {a.x * s, a.y * s, a.z * s};
}

float dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 cross(Vec3 a, Vec3 b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

Vec3 normalize(Vec3 v)
{
    return v * (1.0f / std::sqrt(dot(v, v)));
}

Mat4 modelMatrix(Vec3 translation, float rotationDegrees, Vec3 scale)
{
    float angle = rotationDegrees * pi / 180;
    float c = std::cos(angle);
    float s = std::sin(angle);

    return {
        c * scale.x, 0, -s * scale.x, 0,
        0, scale.y, 0, 0,
        s * scale.z, 0, c * scale.z, 0,
        translation.x, translation.y, translation.z, 1
    };
}

Mat4 partModelMatrix(Vec3 origin, float rotationDegrees, Vec3 localCenter, Vec3 scale)
{
    float angle = rotationDegrees * pi / 180.0f;
    float c = std::cos(angle);
    float s = std::sin(angle);
    Vec3 rotatedCenter{
        c * localCenter.x + s * localCenter.z,
        localCenter.y,
        -s * localCenter.x + c * localCenter.z
    };

    return modelMatrix(origin + rotatedCenter, rotationDegrees, scale);
}

Mat4 projectionMatrix(float aspect)
{
    // Perspective projection: 60-degree vertical field of view, near/far clipping planes.
    constexpr float n = 0.1f, f = 600;
    float scale = 1 / std::tan(60 * pi / 360);

    return {
        scale / aspect, 0, 0, 0,
        0, scale, 0, 0,
        0, 0, (f + n) / (n - f), -1,
        0, 0, 2 * f * n / (n - f), 0
    };
}
