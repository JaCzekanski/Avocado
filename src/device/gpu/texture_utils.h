#pragma once
#include "gpu.h"

#define gpuVRAM ((uint16_t(*)[VRAM_WIDTH])gpu->vram.data())

inline uint16_t tex4bit(GPU* gpu, glm::ivec2 tex, glm::ivec2 texPage, glm::ivec2 clut) {
    uint16_t index = gpuVRAM[texPage.y + tex.y][texPage.x + tex.x / 4];
    uint16_t entry = (index >> ((tex.x & 3) * 4)) & 0xf;
    return gpuVRAM[clut.y][clut.x + entry];
}

inline uint16_t tex8bit(GPU* gpu, glm::ivec2 tex, glm::ivec2 texPage, glm::ivec2 clut) {
    uint16_t index = gpuVRAM[texPage.y + tex.y][texPage.x + tex.x / 2];
    uint16_t entry = (index >> ((tex.x & 1) * 8)) & 0xff;
    return gpuVRAM[clut.y][clut.x + entry];
}

#undef gpuVRAM