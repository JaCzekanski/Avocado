#pragma once
#include <glad/glad.h>

class Buffer {
    GLuint id;

   public:
    static GLuint currentId;

    Buffer(int size);
    ~Buffer();

    void update(int size, const void* data);
    void bind();
    GLuint get();
};