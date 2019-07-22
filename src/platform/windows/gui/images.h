#pragma once
#include <opengl.h>
#include <optional>
#include <string>

struct Image {
    GLuint id;
    int w;
    int h;

    Image(GLuint id, int w, int h);
};

std::optional<Image> getImage(const std::string& button, const std::string& path = "data/assets");