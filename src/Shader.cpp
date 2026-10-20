#include "Shader.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    std::string readText(const std::filesystem::path& file)
    {
        std::ifstream input(file);

        if (!input)
        {
            throw std::runtime_error("Cannot open shader: " + file.string());
        }

        std::ostringstream text;
        text << input.rdbuf();

        return text.str();
    }

    GLuint compileShader(GLenum type, const std::filesystem::path& file)
    {
        std::string source = readText(file);
        const char* ptr = source.c_str();

        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &ptr, nullptr);
        glCompileShader(shader);

        // Report the driver's GLSL compiler log if compilation fails.
        GLint ok = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);

        if (!ok)
        {
            GLint size = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);

            std::vector<char> log(static_cast<size_t>(std::max(size, 1)));
            glGetShaderInfoLog(shader, size, nullptr, log.data());
            glDeleteShader(shader);

            throw std::runtime_error(file.string() + ": " + log.data());
        }

        return shader;
    }

    GLuint createProgram(const std::filesystem::path& shaders)
    {
        GLuint vs = compileShader(GL_VERTEX_SHADER, shaders / "ground.vert"), fs = 0;

        try
        {
            fs = compileShader(GL_FRAGMENT_SHADER, shaders / "ground.frag");
        }
        catch (...)
        {
            glDeleteShader(vs);
            throw;
        }

        GLuint program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        // The linked program retains the shaders; their separate objects can be deleted.
        glDeleteShader(vs);
        glDeleteShader(fs);

        GLint ok = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &ok);

        if (!ok)
        {
            GLint size = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &size);

            std::vector<char> log(static_cast<size_t>(std::max(size, 1)));
            glGetProgramInfoLog(program, size, nullptr, log.data());
            glDeleteProgram(program);

            throw std::runtime_error(std::string("Shader link failed: ") + log.data());
        }

        return program;
    }
}

Shader::Shader(const std::filesystem::path& shaderDirectory)
    : program(createProgram(shaderDirectory))
{
}

Shader::~Shader()
{
    glDeleteProgram(program);
}

void Shader::use() const
{
    glUseProgram(program);
}

GLint Shader::uniformLocation(const char* name) const
{
    return glGetUniformLocation(program, name);
}
