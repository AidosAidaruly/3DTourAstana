#pragma once
#include "Math3D.h"
#include <glad/glad.h>

// The unmeshed envelope is shared with triangular infill. q=0/1 are
// anchored to the two curb edges; q=.5 is the gently flattened crown.
namespace AtyrauCanopyShape
{
    constexpr int ribCount=31;
    constexpr float footHalfWidth=5.94f,footHeight=.25f;
    Vec3 surface(float t,float q);
}

// Primary steel, triangular infill and folded central scales share static
// indexed buffers. Three opaque ranges; no transparency or per-frame rebuilds.
class AtyrauCanopy
{
public:
    AtyrauCanopy();
    ~AtyrauCanopy();
    AtyrauCanopy(const AtyrauCanopy&)=delete;
    AtyrauCanopy& operator=(const AtyrauCanopy&)=delete;
    void render(GLint modelLocation,GLint objectColorLocation,
                GLint useObjectColorLocation,GLint surfaceLocation) const;
private:
    GLuint vao=0,vbo=0,ebo=0;
    GLsizei indexCount=0,latticeIndexCount=0,panelIndexCount=0;
};
