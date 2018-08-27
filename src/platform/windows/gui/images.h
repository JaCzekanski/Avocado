#pragma once
#include <string>
#include <glad/glad.h>

#ifdef __APPLE__
#include <experimental/optional>
using std::experimental::optional;
#else
#include <optional>
using std::optional;
#endif

struct Image {
    GLuint id;
    int w;
    int h;

    Image(GLuint id, int w, int h);
};

optional<Image> getImage(const std::string& button, const std::string& path = "data/assets");