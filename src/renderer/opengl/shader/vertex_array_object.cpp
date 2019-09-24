#include "vertex_array_object.h"

GLuint VertexArrayObject::currentId = 0;

VertexArrayObject::VertexArrayObject() { glGenVertexArrays(1, &id); }

VertexArrayObject::~VertexArrayObject() { glDeleteVertexArrays(1, &id); }

void VertexArrayObject::bind() {
    if (currentId != id) {
        currentId = id;
        glBindVertexArray(id);
    }
}

GLuint VertexArrayObject::get() { return id; }