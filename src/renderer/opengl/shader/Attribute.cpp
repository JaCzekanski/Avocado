#include "Attribute.h"

Attribute::Attribute(GLint id) : id(id) {}

Attribute::~Attribute() {}

void Attribute::enable() { glEnableVertexAttribArray(id); }

void Attribute::disable() { glDisableVertexAttribArray(id); }

void Attribute::pointer(GLint size, GLenum type, GLsizei stride, int pointer) {
    enable();
    if (type == GL_INT || type == GL_UNSIGNED_INT)
        glVertexAttribIPointer(id, size, type, stride, (const GLvoid*)pointer);
    else
        glVertexAttribPointer(id, size, type, false, stride, (const GLvoid*)pointer);
}