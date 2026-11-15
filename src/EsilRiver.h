#pragma once
#include "Math3D.h"
#include <glad/glad.h>
#include <array>
#include <cstddef>

// Static cut-out terrain, recessed banks and opaque animated water. The layout
// is shared with future bridge work; no new transparency pass is required.
class EsilRiver
{
public:
    struct Section {float centerX,width;};
    static constexpr float minZ=-600,maxZ=700,waterY=-3.6f,bankWidth=24;
    static constexpr float terrainMinX=-1500,terrainMaxX=800,terrainMinZ=-1000,terrainMaxZ=950;
    static Section sectionAt(float z);
    // Flat-land offset curves and the existing piecewise-linear slope height
    // let waterfront additions attach without rebuilding the approved river.
    static Vec3 upperBankPoint(float z,int side,float offset=0);
    static Vec3 upperBankEndPoint(int end,float t,float offset=0);
    static float bankSurfaceHeight(float distance);
    EsilRiver();
    ~EsilRiver();
    EsilRiver(const EsilRiver&)=delete;
    EsilRiver& operator=(const EsilRiver&)=delete;
    void renderTerrain(GLint modelLocation) const;
    void renderRiver(GLint modelLocation,GLint objectColorLocation,
                     GLint useObjectColorLocation,GLint surfaceLocation) const;
private:
    struct Range {std::size_t first=0;GLsizei count=0;};
    std::array<Range,3> ranges{};
    GLuint vao=0,vbo=0,ebo=0;
};
