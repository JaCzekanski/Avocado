#pragma once
#include <opengl.h>
#include <string>

enum class ShaderType { Vertex, Fragment };

class Shader {
   public:
    static const char* header;

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
