#pragma once
#include <opengl.h>
#include <unordered_map>
#include <vector>
#include "attribute.h"
#include "shader.h"
#include "uniform.h"

class Program {
    std::string name;
    std::string error = "";
    std::vector<Shader> shaders;
    GLuint programId = 0;
    std::unordered_map<const char*, Attribute> attributes;
    std::unordered_map<const char*, Uniform> uniforms;

    void destroy();
    GLuint link(std::vector<Shader>& shaders);
    bool initialized;

   public:
    Program(std::string name);
    ~Program();

    std::string getName() { return name; }

    bool load();
    std::string getError();

    GLuint get();
    bool use();
    Attribute getAttrib(const char* name);
    Uniform getUniform(const char* name);
};
