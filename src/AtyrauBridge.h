#pragma once
#include "Math3D.h"
#include <glad/glad.h>
#include <array>
#include <cstddef>

// Shared parametric reference for this structural base and the future canopy.
// sample(t).center is the finished walking surface; right stays horizontal.
namespace AtyrauAlignment
{
    constexpr float startX=391.5f,endX=628.5f,centerZ=100,width=12;
    constexpr float landingY=.04f,midspanRise=3.6f;
    struct Frame {Vec3 center,tangent,right,normal;};
    Frame sample(float t);
    float parameterAtX(float x);
    // Cached top triangles, queried only in the bridge footprint. Returns
    // the existing ground datum outside it; no per-frame geometry or uploads.
    float walkingSurfaceHeight(Vec3 position);
}

class AtyrauBridge
{
public:
    AtyrauBridge();
    ~AtyrauBridge();
    AtyrauBridge(const AtyrauBridge&)=delete;
    AtyrauBridge& operator=(const AtyrauBridge&)=delete;
    void render(GLint modelLocation,GLint objectColorLocation,
                GLint useObjectColorLocation,GLint surfaceLocation) const;
private:
    struct Range {std::size_t first=0;GLsizei count=0;};
    std::array<Range,4> ranges{};
    GLuint vao=0,vbo=0,ebo=0;
};
