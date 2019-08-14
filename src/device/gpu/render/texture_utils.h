#pragma once
#include "device/gpu/gpu.h"
#include "utils/macros.h"

#define gpuVRAM ((uint16_t(*)[gpu::VRAM_WIDTH])gpu->vram.data())

enum class ColorDepth { NONE, BIT_4, BIT_8, BIT_16 };

namespace {
// Using unsigned vectors allows compiler to generate slightly faster division code
INLINE uint16_t tex4bit(gpu::GPU* gpu, glm::uvec2 tex, glm::uvec2 texPage, glm::uvec2 clut) {
    uint16_t index = gpuVRAM[texPage.y + tex.y][texPage.x + tex.x / 4];
    uint8_t entry = (index >> ((tex.x & 3) * 4)) & 0xf;
    return gpuVRAM[clut.y][clut.x + entry];
}

INLINE uint16_t tex8bit(gpu::GPU* gpu, glm::uvec2 tex, glm::uvec2 texPage, glm::uvec2 clut) {
    uint16_t index = gpuVRAM[texPage.y + tex.y][texPage.x + tex.x / 2];
    uint8_t entry = (index >> ((tex.x & 1) * 8)) & 0xff;
    return gpuVRAM[clut.y][clut.x + entry];
}

INLINE uint16_t tex16bit(gpu::GPU* gpu, glm::uvec2 tex, glm::uvec2 texPage) { return gpuVRAM[texPage.y + tex.y][texPage.x + tex.x]; }

template <ColorDepth bits>
INLINE PSXColor fetchTex(gpu::GPU* gpu, glm::uvec2 texel, const glm::ivec2 texPage, const glm::ivec2 clut) {
    if constexpr (bits == ColorDepth::BIT_4) {
        return tex4bit(gpu, texel, texPage, clut);
    } else if constexpr (bits == ColorDepth::BIT_8) {
        return tex8bit(gpu, texel, texPage, clut);
    } else if constexpr (bits == ColorDepth::BIT_16) {
        return tex16bit(gpu, texel, texPage);
    } else {
        static_assert(true, "Invalid ColorDepth parameter");
    }
}
};  // namespace

#undef gpuVRAM