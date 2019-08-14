#pragma once
#include <glm/glm.hpp>

struct Rectangle {
    glm::ivec2 pos;
    glm::ivec2 size;
    glm::ivec3 color;

    // Flags
    int bits;
    bool isSemiTransparent;
    bool isRawTexture;

    // Texture, valid if bits != 0
    glm::ivec2 uv;
    glm::ivec2 texpage;  // Texture page position in VRAM (from GP0_E1)
    glm::ivec2 clut;     // Texture palette position in VRAM
};