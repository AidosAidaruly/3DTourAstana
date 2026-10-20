#pragma once

#include <glad/glad.h>

#include <filesystem>

class Shader
{
public:
    explicit Shader(const std::filesystem::path& shaderDirectory);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;
    GLint uniformLocation(const char* name) const;

private:
    GLuint program = 0;
};
