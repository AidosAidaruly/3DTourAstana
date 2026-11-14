#pragma once
#include "Math3D.h"
#include <glad/glad.h>
#include <array>
#include <cstddef>
#include <cmath>
#include <vector>

namespace MegaSilkWaySite
{
    constexpr Vec3 center{-950,0,-645};
    constexpr float minX=-1180,maxX=-720,minZ=-910,maxZ=-440;
    constexpr float junctionX=-770,junctionZ=-410;
    constexpr Vec3 arrivalCenter{-895,0,-444};
    constexpr float mallLength=340,mallWidth=160,podiumLength=348,podiumWidth=168;
    // The site frame preserves the approved roads and plot. Building rotation
    // is separate, so changing its front never rotates the access network.
    constexpr Vec3 layoutOrigin{-580,0,385};
    // Exact world-Z longitudinal axis. The 160 m front is parallel to world X,
    // centered at (-950,-475); the rear short end is centered at (-950,-815).
    constexpr Vec3 frontDirection() {return {0,0,1};}
    inline Mat4 placement()
    {
        // Exact half turn avoids trigonometric roundoff at the shared road seam.
        return {-1,0,0,0, 0,1,0,0, 0,0,-1,0, center.x,center.y,center.z,1};
    }
    // Only the southern frontage's outer sidewalk opens at the new junction.
    inline bool atJunction(Vec3 p,float margin=0)
    {return std::abs(p.z-junctionZ)<.01f&&p.x>junctionX-14-margin&&p.x<junctionX+14+margin;}
    // Parking entry opens only the west edge of the existing boulevard.
    inline bool atParkingJunction(Vec3 p,float margin=0)
    {return std::abs(p.x+1060)<.01f&&p.z>-362-margin&&p.z<-338+margin;}
}

// Approved shell with a batched opaque frame and a separately sorted glass roof.
// Architecture, plaza and parking share static material batches.
class MegaSilkWay
{
public:
    MegaSilkWay();
    ~MegaSilkWay();
    MegaSilkWay(const MegaSilkWay&)=delete;
    MegaSilkWay& operator=(const MegaSilkWay&)=delete;
    void render(GLint modelLocation,GLint colorLocation,GLint textureLocation,
                GLint repeatLocation,GLint asphaltLocation,GLuint pavingTexture) const;
    void renderGlass(GLint modelLocation,GLint colorLocation,GLint atriumLocation,
                     const Mat4& view) const;
private:
    struct Range {std::size_t first=0;GLsizei count=0;};
    Mat4 model=MegaSilkWaySite::placement();
    GLuint vao=0,vbo=0,ebo=0,glassEbo=0;
    GLuint plazaStoneTexture=0;
    std::array<Range,23> ranges;
    struct GlassTriangle {std::array<GLuint,3> indices;Vec3 worldCenter;};
    std::vector<GlassTriangle> glassTriangles;
    struct GlassSortKey {double depth=0;std::size_t triangle=0;};
    mutable std::vector<GlassSortKey> glassOrder;
    mutable std::vector<GLuint> glassSortedIndices;
    mutable Vec3 glassSortedDirection{};
    mutable bool glassOrderValid=false;
    mutable GLuint materialProgram=0;
    mutable GLint glassLocation=-1;
};
