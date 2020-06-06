#pragma once
#include <opengl.h>
#include <optional>
#include <string>
#include <config.h>

struct Image {
    GLuint id;
    int w;
    int h;

    Image(GLuint id, int w, int h);
};

std::optional<Image> getImage(const std::string& image, const std::string& path = avocado::assetsPath());