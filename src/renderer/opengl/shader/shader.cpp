#include "shader.h"
#include <array>
#include <vector>
#include "utils/file.h"

#ifdef USE_OPENGLES
const char* Shader::header = R"EOF(#version 300 es
#ifdef GL_ES
precision mediump float;
#endif

)EOF";
#else
const char* Shader::header = R"EOF(#version 150

)EOF";
#endif

Shader::Shader(std::string name, ShaderType shaderType) {
    this->name = name;
    this->type = shaderType;
}

Shader::~Shader() { destroy(); }

void Shader::destroy() {
    if (compiled && shaderId != 0) glDeleteShader(shaderId);
}

bool Shader::reload() {
    destroy();
    return compile();
}

bool Shader::compile() {
    error = name + ": ";
    auto data = getFileContentsAsString(name);

    std::array<const char*, 2> lines = {{header, data.c_str()}};

    if (type == ShaderType::Vertex) {
        shaderId = glCreateShader(GL_VERTEX_SHADER);
    } else if (type == ShaderType::Fragment) {
        shaderId = glCreateShader(GL_FRAGMENT_SHADER);
    } else {
        error = std::string("Wrong shader type");
        return false;
    }
    if (shaderId == 0) {
        error = std::string("Cannot create shader");
        return false;
    }

    glShaderSource(shaderId, lines.size(), lines.data(), nullptr);
    glCompileShader(shaderId);

    GLint status;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &status);
    if (!status) {
        std::string buffer;
        buffer.resize(1024);
        glGetShaderInfoLog(shaderId, 1024, nullptr, &buffer[0]);
        error += buffer;
        return false;
    }
    error = std::string();
    compiled = true;
    return true;
}

GLuint Shader::get() { return shaderId; }

std::string Shader::getError() { return error; }

bool Shader::isCompiled() { return compiled; }