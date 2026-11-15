#pragma once
#include "Math3D.h"
#include <glad/glad.h>
#include <array>
#include <cstddef>

// Initialization-only, material-batched waterfront additions. The approved
// river, slopes and surrounding ground are owned separately by EsilRiver.
class EsilEmbankment
{
public:
    static constexpr Vec3 riverViewEye{384,3,100};
    static constexpr Vec3 riverViewTarget{510,-1,100};
    EsilEmbankment();
    ~EsilEmbankment();
    EsilEmbankment(const EsilEmbankment&)=delete;
    EsilEmbankment& operator=(const EsilEmbankment&)=delete;
    void render(GLint modelLocation,GLint objectColorLocation,
                GLint useObjectColorLocation,GLint surfaceLocation) const;
private:
    struct Range {std::size_t first=0;GLsizei count=0;};
    std::array<Range,15> ranges{};
    GLuint vao=0,vbo=0,ebo=0;
};
