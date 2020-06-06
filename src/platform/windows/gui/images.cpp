#include "images.h"
#include <fmt/core.h>
#include <stb_image.h>
#include <unordered_map>
#include <config.h>
#include "utils/file.h"

Image::Image(GLuint id, int w, int h) : id(id), w(w), h(h) {}

namespace {
std::unordered_map<std::string, std::optional<Image>> images;

std::optional<Image> loadImage(const std::string& file) {
    int w, h, bit;

    auto rawdata = getFileContents(file);
    if (rawdata.empty()) {
        fmt::print("[ERROR] Cannot load {}\n", getFilenameExt(file));
        return {};
    }

    auto data = stbi_load_from_memory(rawdata.data(), rawdata.size(), &w, &h, &bit, 4);
    if (data == nullptr) {
        fmt::print("[ERROR] Cannot load {}\n", getFilenameExt(file));
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

std::optional<Image> getImage(const std::string& filename, const std::string& path) {
    auto image = images.find(path + filename);
    if (image != images.end()) {
        return image->second;
    }

    auto loaded = loadImage(fmt::format("{}/{}.png", path, filename));
    images[path + filename] = loaded;

    return loaded;
}