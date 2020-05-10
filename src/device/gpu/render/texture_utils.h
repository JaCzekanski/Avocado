#pragma once
#include "device/gpu/gpu.h"
#include "utils/macros.h"
#include "../color_depth.h"
#include "../primitive.h"

#define gpuVRAM ((uint16_t(*)[gpu::VRAM_WIDTH])gpu->vram.data())

template <ColorDepth bits>
void loadClutCacheIfRequired(gpu::GPU* gpu, ivec2 clut) {
    // Only paletted textures should reload the color look-up table cache
    if constexpr (bits != ColorDepth::BIT_4 && bits != ColorDepth::BIT_8) {
        return;
    }

    bool textureFormatRequireReload = bits > gpu->clutCacheColorDepth;
    bool clutPositionChanged = gpu->clutCachePos != clut;

    if (!textureFormatRequireReload && !clutPositionChanged) {
        return;
    }

    gpu->clutCacheColorDepth = bits;
    gpu->clutCachePos = clut;

    constexpr int entries = (bits == ColorDepth::BIT_8) ? 256 : 16;
    for (int i = 0; i < entries; i++) {
        gpu->clutCache[i] = gpuVRAM[clut.y][clut.x + i];
    }
}

namespace {
INLINE uint16_t tex4bit(gpu::GPU* gpu, ivec2 tex, ivec2 texPage) {
    uint16_t index = gpuVRAM[(texPage.y + tex.y) & 511][(texPage.x + tex.x / 4) & 1023];
    uint8_t entry = (index >> ((tex.x & 3) * 4)) & 0xf;
    return gpu->clutCache[entry];
}

INLINE uint16_t tex8bit(gpu::GPU* gpu, ivec2 tex, ivec2 texPage) {
    uint16_t index = gpuVRAM[(texPage.y + tex.y) & 511][(texPage.x + tex.x / 2) & 1023];
    uint8_t entry = (index >> ((tex.x & 1) * 8)) & 0xff;
    return gpu->clutCache[entry];
}

INLINE uint16_t tex16bit(gpu::GPU* gpu, ivec2 tex, ivec2 texPage) { return gpuVRAM[(texPage.y + tex.y) & 511][(texPage.x + tex.x) & 1023]; }

template <ColorDepth bits>
INLINE PSXColor fetchTex(gpu::GPU* gpu, ivec2 texel, const ivec2 texPage) {
    if constexpr (bits == ColorDepth::BIT_4) {
        return tex4bit(gpu, texel, texPage);
    } else if constexpr (bits == ColorDepth::BIT_8) {
        return tex8bit(gpu, texel, texPage);
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