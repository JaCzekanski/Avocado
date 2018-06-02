#pragma once
#include "gpu.h"
#include "utils/macros.h"

#define gpuVRAM ((uint16_t(*)[VRAM_WIDTH])gpu->vram.data())

enum class ColorDepth { NONE, BIT_4, BIT_8, BIT_16 };

INLINE uint16_t tex4bit(GPU* gpu, glm::ivec2 tex, glm::ivec2 texPage, glm::ivec2 clut) {
    uint16_t index = gpuVRAM[texPage.y + tex.y][texPage.x + tex.x / 4];
    uint16_t entry = (index >> ((tex.x & 3) * 4)) & 0xf;
    return gpuVRAM[clut.y][clut.x + entry];
}

INLINE uint16_t tex8bit(GPU* gpu, glm::ivec2 tex, glm::ivec2 texPage, glm::ivec2 clut) {
    uint16_t index = gpuVRAM[texPage.y + tex.y][texPage.x + tex.x / 2];
    uint16_t entry = (index >> ((tex.x & 1) * 8)) & 0xff;
    return gpuVRAM[clut.y][clut.x + entry];
}

INLINE uint16_t tex16bit(GPU* gpu, glm::ivec2 tex, glm::ivec2 texPage) {
    return gpuVRAM[texPage.y + tex.y][texPage.x + tex.x];
    // TODO: In PSOne BIOS colors are swapped (r == b, g == g, b == r, k == k)
}

#undef gpuVRAM