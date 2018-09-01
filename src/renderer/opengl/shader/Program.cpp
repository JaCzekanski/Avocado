#include "program.h"
#include "utils/file.h"

Program::Program(std::string name) { this->name = name; }

Program::~Program() { destroy(); }

void Program::destroy() {
    if (programId == 0) return;
    glDeleteProgram(programId);
    shaders.clear();
}

bool Program::load() {
    if (!fileExists(name + ".frag") || !fileExists(name + ".vert")) {
        error = std::string("File doesn't exists.");
        return false;
    }

    std::vector<Shader> newShaders;

    newShaders.push_back(Shader(name + ".frag", ShaderType::Fragment));
    newShaders.push_back(Shader(name + ".vert", ShaderType::Vertex));

    GLuint id = link(newShaders);
    if (id == 0) return false;

    shaders = move(newShaders);
    programId = id;
    initialized = true;
    return true;
}

GLuint Program::link(std::vector<Shader>& shaders) {
    GLuint id = glCreateProgram();
    if (id == 0) return 0;

    for (Shader& s : shaders) {
        if (s.isCompiled()) continue;
        if (!s.compile()) {
            error = s.getError();
            return false;
        }
        glAttachShader(id, s.get());
    }

    glLinkProgram(id);

    GLint status;
    glGetProgramiv(id, GL_LINK_STATUS, &status);
    if (status == false) {
        std::string buffer;
        buffer.resize(1024);
        glGetProgramInfoLog(id, 1024, nullptr, &buffer[0]);
        error = "Linking error: " + buffer;

        glDeleteProgram(id);
        return 0;
    }
    return id;
}

GLuint Program::get() { return programId; }

std::string Program::getError() { return error; }

bool Program::use() {
    if (!initialized) return false;
    if (programId == 0) {
        error = std::string("Program not linked.");
        return false;
    }
    glUseProgram(programId);
    return true;
}

Attribute Program::getAttrib(const char* name) {
    if (!initialized) return 0;
    if (auto key = attributes.find(name); key != attributes.end()) {
        return key->second;
    }
    GLint loc = glGetAttribLocation(programId, name);
    if (loc == -1) {
        printf("[GL] Cannot find attribute \"%s\" in program %s\n", name, this->name.c_str());
    }
    auto attribute = Attribute(loc);
    attributes.emplace(name, attribute);

    return attribute;
}

Uniform Program::getUniform(const char* name) {
    if (!initialized) return 0;
    if (auto key = uniforms.find(name); key != uniforms.end()) {
        return key->second;
    }
    GLint loc = glGetUniformLocation(programId, name);
    if (loc == -1) {
        printf("[GL] Cannot find uniform \"%s\" in program %s\n", name, this->name.c_str());
    }
    auto uniform = Uniform(loc);
    uniforms.emplace(name, uniform);

    return uniform;
}