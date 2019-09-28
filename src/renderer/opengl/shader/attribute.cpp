#include "attribute.h"

Attribute::Attribute(GLuint id) : id(id) {}

Attribute::~Attribute() {}

void Attribute::enable() { glEnableVertexAttribArray(id); }

void Attribute::disable() { glDisableVertexAttribArray(id); }

void Attribute::pointer(GLint size, GLenum type, GLsizei stride, uintptr_t pointer) {
    enable();
    if (type == GL_BYTE || type == GL_UNSIGNED_BYTE || type == GL_SHORT || type == GL_UNSIGNED_SHORT || type == GL_INT
        || type == GL_UNSIGNED_INT) {
        glVertexAttribIPointer(id, size, type, stride, (const GLvoid*)pointer);
    } else {
        glVertexAttribPointer(id, size, type, false, stride, (const GLvoid*)pointer);
    }
}

size_t Attribute::getSize(GLenum type) {
    switch (type) {
        case GL_BYTE: return sizeof(char);
        case GL_UNSIGNED_BYTE: return sizeof(unsigned char);
        case GL_SHORT: return sizeof(short);
        case GL_UNSIGNED_SHORT: return sizeof(unsigned short);
        case GL_INT: return sizeof(int);
        case GL_UNSIGNED_INT: return sizeof(unsigned int);
        case GL_FLOAT: return sizeof(float);
        case GL_DOUBLE: return sizeof(double);
    }
}