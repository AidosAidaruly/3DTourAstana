#pragma once
#include <glad/glad.h>

// One packed, mipmapped detail map shared by ground materials only.
class GroundMaterials
{
public:
    GroundMaterials();
    ~GroundMaterials();
    GroundMaterials(const GroundMaterials&)=delete;
    GroundMaterials& operator=(const GroundMaterials&)=delete;
    static void configure(GLuint program);
    static const char* shaderLibrary();
    class Binding
    {
    public:
        explicit Binding(const GroundMaterials& materials);
        ~Binding();
        Binding(const Binding&)=delete;
        Binding& operator=(const Binding&)=delete;
    private:
        GLint active=0,previous=0;
    };
private:
    GLuint texture=0;
};
