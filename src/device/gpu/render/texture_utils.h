#pragma once
#include "device/gpu/gpu.h"
#include "utils/macros.h"
#include "../primitive.h"

#define gpuVRAM ((uint16_t(*)[gpu::VRAM_WIDTH])gpu->vram.data())

enum class ColorDepth { NONE, BIT_4, BIT_8, BIT_16 };

ColorDepth bitsToDepth(int bits);

template <int bits>
constexpr ColorDepth bitsToDepth() {
    if constexpr (bits == 4)
        return ColorDepth::BIT_4;
    else if constexpr (bits == 8)
        return ColorDepth::BIT_8;
    else if constexpr (bits == 16)
        return ColorDepth::BIT_16;
    else
        return ColorDepth::NONE;
}

namespace {
// Using unsigned vectors allows compiler to generate slightly faster division code
INLINE uint16_t tex4bit(gpu::GPU* gpu, ivec2 tex, ivec2 texPage, ivec2 clut) {
    uint16_t index = gpuVRAM[(texPage.y + tex.y) & 511][(texPage.x + tex.x / 4) & 1023];
    uint8_t entry = (index >> ((tex.x & 3) * 4)) & 0xf;
    return gpuVRAM[clut.y][clut.x + entry];
}

INLINE uint16_t tex8bit(gpu::GPU* gpu, ivec2 tex, ivec2 texPage, ivec2 clut) {
    uint16_t index = gpuVRAM[(texPage.y + tex.y) & 511][(texPage.x + tex.x / 2) & 1023];
    uint8_t entry = (index >> ((tex.x & 1) * 8)) & 0xff;
    return gpuVRAM[clut.y][clut.x + entry];
}

INLINE uint16_t tex16bit(gpu::GPU* gpu, ivec2 tex, ivec2 texPage) { return gpuVRAM[(texPage.y + tex.y) & 511][(texPage.x + tex.x) & 1023]; }

template <ColorDepth bits>
INLINE PSXColor fetchTex(gpu::GPU* gpu, ivec2 texel, const ivec2 texPage, const ivec2 clut) {
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

INLINE ivec2 maskTexel(ivec2 texel, const gpu::GP0_E2 textureWindow) {
    // Texture is repeated outside of 256x256 window
    texel.x %= 256u;
    texel.y %= 256u;

    // Texture masking
    // texel = (texel AND(NOT(Mask * 8))) OR((Offset AND Mask) * 8)
    texel.x = (texel.x & ~(textureWindow.maskX * 8)) | ((textureWindow.offsetX & textureWindow.maskX) * 8);
    texel.y = (texel.y & ~(textureWindow.maskY * 8)) | ((textureWindow.offsetY & textureWindow.maskY) * 8);

    return texel;
}
};  // namespace

#undef gpuVRAM