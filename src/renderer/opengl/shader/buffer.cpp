#include "buffer.h"

GLuint Buffer::currentId = 0;

Buffer::Buffer(size_t size) {
    GLint lastBuffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &lastBuffer);

    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, lastBuffer);
}

Buffer::~Buffer() { glDeleteBuffers(1, &id); }

void Buffer::update(int size, const void* data) {
    bind();
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}

void Buffer::bind() {
    if (currentId != id) {
        currentId = id;
        glBindBuffer(GL_ARRAY_BUFFER, id);
    }
}

GLuint Buffer::get() { return id; }