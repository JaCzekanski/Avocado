#pragma once
#include <tuple>
#include "glad/glad.h"

class Uniform {
    GLuint id;

   public:
    Uniform(GLuint id);
    ~Uniform();

    // TODO: template ?
    void i(GLint v0);
    void i(GLint v0, GLint v1);
    void i(GLint v0, GLint v1, GLint v2);
    void i(GLint v0, GLint v1, GLint v2, GLint v3);
    void u(GLuint v0);
    void u(GLuint v0, GLuint v1);
    void u(GLuint v0, GLuint v1, GLuint v2);
    void u(GLuint v0, GLuint v1, GLuint v2, GLuint v3);
    void f(GLfloat v0);
    void f(GLfloat v0, GLfloat v1);
    void f(GLfloat v0, GLfloat v1, GLfloat v2);
    void f(GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
};
