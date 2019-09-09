#include "vertex_array_object.h"

GLuint VertexArrayObject::currentId = 0;

VertexArrayObject::VertexArrayObject() {
    GLint lastArray;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &lastArray);

    glGenVertexArrays(1, &id);

    glBindBuffer(GL_ARRAY_BUFFER, lastArray);
}

VertexArrayObject::~VertexArrayObject() { glDeleteVertexArrays(1, &id); }

void VertexArrayObject::bind() {
    if (currentId != id) {
        currentId = id;
        glBindVertexArray(id);
    }
}

GLuint VertexArrayObject::get() { return id; }