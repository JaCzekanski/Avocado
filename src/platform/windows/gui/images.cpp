#include "images.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <unordered_map>
#include "utils/file.h"
#include "utils/string.h"

Image::Image(GLuint id, int w, int h) : id(id), w(w), h(h) {}

namespace {
std::unordered_map<std::string, optional<Image>> images;

optional<Image> loadImage(const std::string& file) {
    int w, h, bit;
    auto data = stbi_load(file.c_str(), &w, &h, &bit, 4);
    if (data == nullptr) {
        printf("[Error] Cannot load %s\n", getFilenameExt(file).c_str());
        return {};
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);

    glBindTexture(GL_TEXTURE_2D, 0);
    return Image(tex, w, h);
}

}  // namespace

optional<Image> getImage(const std::string& button, const std::string& path) {
    auto image = images.find(path + button);
    if (image != images.end()) {
        return image->second;
    }

    auto loaded = loadImage(string_format("%s/%s.png", path.c_str(), button.c_str()));
    images[path + button] = loaded;

    return loaded;
}