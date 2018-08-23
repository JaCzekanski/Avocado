#pragma once
#include <string>
#include <experimental/optional>
#include <glad/glad.h>

struct Image {
    GLuint id;
    int w;
    int h;

    Image(GLuint id, int w, int h);
};

std::experimental::optional<Image> getImage(const std::string& button, const std::string& path = "data/assets");