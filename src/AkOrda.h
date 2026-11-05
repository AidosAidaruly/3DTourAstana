#pragma once

#include "Math3D.h"
#include "Mesh.h"

#include <glad/glad.h>

#include <vector>

// Symmetric Ak Orda palace with a classical front facade and blue dome.
class AkOrda
{
public:
    explicit AkOrda(Vec3 position);
    ~AkOrda();

    AkOrda(const AkOrda&) = delete;
    AkOrda& operator=(const AkOrda&) = delete;

    void render(const Mesh& mesh, GLint modelLocation,
                GLint objectColorLocation, GLint stoneMaterialLocation,
                GLint roofMaterialLocation) const;

private:
    struct Part
    {
        Mat4 model;
        Vec3 color;
        bool stoneMaterial;
    };

    struct VertexRange
    {
        GLint first = 0;
        GLsizei count = 0;
    };

    void drawRange(const VertexRange& range, Vec3 color,
                   GLint objectColorLocation) const;

    Mat4 landmarkModel;
    Mat4 finialModel;
    std::vector<Part> stoneParts;
    std::vector<Part> facadeParts;
    VertexRange curvedBaseRange;
    VertexRange curvedFacadeRange;
    VertexRange curvedWindowRange;
    VertexRange curvedBandRange;
    VertexRange porticoMiddleRange;
    VertexRange porticoColumnsRange;
    VertexRange wingPilasterRange;
    VertexRange upperCurvedWallRange;
    VertexRange upperWindowRange;
    VertexRange upperCorniceRange;
    VertexRange domeDrumRange;
    VertexRange domeDrumTrimRange;
    VertexRange domeRange;
    VertexRange domeRibRange;
    VertexRange spireRange;
    GLuint vao = 0;
    GLuint vbo = 0;
};
