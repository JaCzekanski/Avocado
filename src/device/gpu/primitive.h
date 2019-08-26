#pragma once
#include <glm/glm.hpp>
#include <utility>
#include "device/gpu/semi_transparency.h"
#include "psx_color.h"

namespace primitive {
struct Rect {
    glm::ivec2 pos;
    glm::ivec2 size;
    RGB color;

    // Flags
    int bits;
    bool isSemiTransparent;
    bool isRawTexture;

    // Texture, valid if bits != 0
    glm::ivec2 uv;
    glm::ivec2 texpage;  // Texture page position in VRAM (from GP0_E1)
    glm::ivec2 clut;     // Texture palette position in VRAM
};

struct Line {
    glm::ivec2 pos[2];
    RGB color[2];

    // Flags
    bool isSemiTransparent;
    bool gouroudShading;
};

struct Triangle {
    struct Vertex {
        glm::ivec2 pos;
        RGB color;      // Valid if bits == 0 or !isRawTexture
        glm::ivec2 uv;  // Valid if bits != 0
    };
    Vertex v[3];

    // Flags
    int bits = 0;
    gpu::SemiTransparency transparency;  // Valid if isSemiTransparent
    bool isSemiTransparent = false;
    bool isRawTexture = false;
    bool gouroudShading = false;

    // Texture, valid if bits != 0
    glm::ivec2 texpage;  // Texture page position in VRAM
    glm::ivec2 clut;     // Texture palette position in VRAM

    void assureCcw() {
        if (isCw()) {
            std::swap(v[1], v[2]);
        }
    }

   private:
    bool isCw() const {
        auto ab = v[1].pos - v[0].pos;
        auto ac = v[2].pos - v[0].pos;
        auto cross = ab.x * ac.y - ab.y * ac.x;

        return cross < 0;
    }
};
}  // namespace primitive