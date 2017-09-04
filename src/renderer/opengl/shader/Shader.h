#pragma once
#include <string>
#include <glad/glad.h>

enum class ShaderType { Vertex, Fragment };

class Shader {
   private:
    std::string error = "";
    std::string name;
    bool compiled = false;
    ShaderType type;
    GLuint shaderId = 0;

    void destroy();

   public:
    Shader(std::string name, ShaderType shaderType);
    ~Shader();

    bool compile();
    GLuint get();

    std::string getError();
    bool isCompiled();

    bool reload();
};
