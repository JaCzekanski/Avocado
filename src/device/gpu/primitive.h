#pragma once
#include <glm/glm.hpp>
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
}  // namespace primitive