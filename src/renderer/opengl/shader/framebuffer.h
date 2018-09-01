#pragma once
#include <glad/glad.h>

class Framebuffer {
    GLuint id;

   public:
    static GLuint currentId;

    Framebuffer(GLuint colorTexture);
    ~Framebuffer();

    void bind();
    GLuint get();
};