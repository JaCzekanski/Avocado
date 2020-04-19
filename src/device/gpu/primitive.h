#pragma once
#include <cmath>
#include <utility>
#include "device/gpu/semi_transparency.h"
#include "psx_color.h"
#include "utils/vector.h"

namespace primitive {
struct Rect {
    ivec2 pos;
    ivec2 size;
    RGB color;

    // Flags
    int bits;
    bool isSemiTransparent;
    bool isRawTexture;

    // Texture, valid if bits != 0
    ivec2 uv;
    ivec2 texpage;  // Texture page position in VRAM (from GP0_E1)
    ivec2 clut;     // Texture palette position in VRAM
};

struct Line {
    ivec2 pos[2];
    RGB color[2];

    // Flags
    bool isSemiTransparent;
    bool gouroudShading;
};

struct Triangle {
    struct Vertex {
        ivec2 pos;
        RGB color;  // Valid if bits == 0 or !isRawTexture
        ivec2 uv;   // Valid if bits != 0
    };
    Vertex v[3];

    // Flags
    int bits = 0;
    gpu::SemiTransparency transparency;  // Valid if isSemiTransparent
    bool isSemiTransparent = false;
    bool isRawTexture = false;
    bool gouroudShading = false;

    // Texture, valid if bits != 0
    ivec2 texpage;  // Texture page position in VRAM
    ivec2 clut;     // Texture palette position in VRAM

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