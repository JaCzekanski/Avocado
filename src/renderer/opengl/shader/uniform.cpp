#include "uniform.h"

Uniform::Uniform(GLuint id) : id(id) {}
Uniform::~Uniform() {}

void Uniform::i(GLint v0) { glUniform1i(id, v0); }

void Uniform::i(GLint v0, GLint v1) { glUniform2i(id, v0, v1); }

void Uniform::i(GLint v0, GLint v1, GLint v2) { glUniform3i(id, v0, v1, v2); }

void Uniform::i(GLint v0, GLint v1, GLint v2, GLint v3) { glUniform4i(id, v0, v1, v2, v3); }

void Uniform::u(GLuint v0) { glUniform1ui(id, v0); }

void Uniform::u(GLuint v0, GLuint v1) { glUniform2ui(id, v0, v1); }

void Uniform::u(GLuint v0, GLuint v1, GLuint v2) { glUniform3ui(id, v0, v1, v2); }

void Uniform::u(GLuint v0, GLuint v1, GLuint v2, GLuint v3) { glUniform4ui(id, v0, v1, v2, v3); }

void Uniform::f(GLfloat v0) { glUniform1f(id, v0); }

void Uniform::f(GLfloat v0, GLfloat v1) { glUniform2f(id, v0, v1); }

void Uniform::f(GLfloat v0, GLfloat v1, GLfloat v2) { glUniform3f(id, v0, v1, v2); }

void Uniform::f(GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) { glUniform4f(id, v0, v1, v2, v3); }