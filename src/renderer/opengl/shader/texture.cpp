#include "texture.h"

Texture::Texture(int width, int height, GLint internalFormat, GLint dataFormat, GLenum type, bool filter)
    : width(width), height(height), dataFormat(dataFormat), type(type), success(false) {
    GLint lastTexture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastTexture);

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, type, nullptr);

    if (glGetError() != GL_NO_ERROR) {
        return;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, lastTexture);

    success = true;
}

Texture::~Texture() { glDeleteTextures(1, &id); }

void Texture::update(const void* data) {
    glBindTexture(GL_TEXTURE_2D, id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, dataFormat, type, data);
}

void Texture::bind(int sampler) {
    glActiveTexture(GL_TEXTURE0 + sampler);
    glBindTexture(GL_TEXTURE_2D, id);
}

GLuint Texture::get() { return id; }

int Texture::getWidth() { return width; }

int Texture::getHeight() { return height; }

bool Texture::isCreated() { return success; }