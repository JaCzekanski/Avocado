#pragma once
#include <opengl.h>

class VertexArrayObject {
    GLuint id;

   public:
    static GLuint currentId;

    VertexArrayObject();
    ~VertexArrayObject();

    void bind();
    GLuint get();
};