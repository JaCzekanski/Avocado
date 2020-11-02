#pragma once
#include <opengl.h>
#include <cstddef>

class Attribute {
    GLuint id;

   public:
    Attribute(GLuint id);
    ~Attribute();

    void enable();
    void disable();
    void pointer(GLint size, GLenum type, GLsizei stride, uintptr_t pointer);

    static size_t getSize(GLenum type);
};
