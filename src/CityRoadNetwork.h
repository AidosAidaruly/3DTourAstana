#pragma once
#include "Math3D.h"
#include <glad/glad.h>
#include <array>
#include <cstddef>

class CityRoadNetwork
{
public:
    CityRoadNetwork();
    ~CityRoadNetwork();
    CityRoadNetwork(const CityRoadNetwork&)=delete;
    CityRoadNetwork& operator=(const CityRoadNetwork&)=delete;
    void render(GLint modelLocation,GLint colorLocation,GLint objectLocation,
                GLint asphaltLocation,GLint grassLocation) const;
private:
    struct Range {std::size_t first=0;GLsizei count=0;};
    std::array<Range,4> ranges{};
    GLuint vao=0,vbo=0,ebo=0;
};
