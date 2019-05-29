#pragma once
#include <opengl.h>

class Attribute {
    GLuint id;

   public:
    Attribute(GLuint id);
    ~Attribute();

    void enable();
    void disable();
    void pointer(GLint size, GLenum type, GLsizei stride, uintptr_t pointer);
};
