#pragma once
#include "glad/glad.h"

class Attribute {
    GLint id;

   public:
    Attribute(GLint id);
    ~Attribute();

    void enable();
    void disable();
    void pointer(GLint size, GLenum type, GLsizei stride, int pointer);
};
