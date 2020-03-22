#include "../primitive.h"
#include "render.h"
#include "texture_utils.h"
#include "utils/macros.h"

using glm::ivec2;
using glm::ivec3;
using glm::uvec2;
using gpu::GPU;

#undef VRAM
#define VRAM ((uint16_t(*)[gpu::VRAM_WIDTH])gpu->vram.data())

// Duplicated code, unify
INLINE glm::uvec2 calculateTexel(glm::ivec2 tex, const gpu::GP0_E2 textureWindow) {
    // Texture is repeated outside of 256x256 window
    tex.x %= 256u;
    tex.y %= 256u;

    // Texture masking
    // texel = (texel AND(NOT(Mask * 8))) OR((Offset AND Mask) * 8)
    tex.x = (tex.x & ~(textureWindow.maskX * 8)) | ((textureWindow.offsetX & textureWindow.maskX) * 8);
    tex.y = (tex.y & ~(textureWindow.maskY * 8)) | ((textureWindow.offsetY & textureWindow.maskY) * 8);

    return tex;
}

template <ColorDepth bits, bool isSemiTransparent, bool isBlended, bool checkMaskBeforeDraw>
INLINE void rectangle(GPU* gpu, const primitive::Rect& rect) {
    // Extract common GPU state
    const auto transparency = gpu->gp0_e1.semiTransparency;
    const bool setMaskWhileDrawing = gpu->gp0_e6.setMaskWhileDrawing;
    const auto textureWindow = gpu->gp0_e2;
    constexpr bool isTextured = bits != ColorDepth::NONE;

    if (rect.size.x >= 1024 || rect.size.y >= 512) return;

    const ivec2 pos(                       //
        rect.pos.x + gpu->drawingOffsetX,  //
        rect.pos.y + gpu->drawingOffsetY   //
    );
    const ivec2 min(              //
        gpu->minDrawingX(pos.x),  //
        gpu->minDrawingY(pos.y)   //
    );
    const ivec2 max(                                //
        gpu->maxDrawingX(pos.x + rect.size.x - 1),  //
        gpu->maxDrawingY(pos.y + rect.size.y - 1)   //
    );

    const ivec2 uv(                   //
        rect.uv.x + (min.x - pos.x),  // Add offset if part of rectange was cut off
        rect.uv.y + (min.y - pos.y)   //
    );
    int uStep = 1, vStep = 1;

    // Texture flipping
    // TODO: Not tested!
    if (gpu->gp0_e1.texturedRectangleXFlip) {
        uStep = -1;
    }
    if (gpu->gp0_e1.texturedRectangleYFlip) {
        vStep = -1;
    }

    int x, y, u, v;
    for (y = min.y, v = uv.y; y <= max.y; y++, v += vStep) {
        for (x = min.x, u = uv.x; x <= max.x; x++, u += uStep) {
            PSXColor bg = VRAM[y][x];
            if constexpr (checkMaskBeforeDraw) {
                if (bg.k) continue;
            }

            PSXColor c;
            if constexpr (bits == ColorDepth::NONE) {
                c = PSXColor(rect.color.r, rect.color.g, rect.color.b);
            } else {
                uvec2 texel = calculateTexel(ivec2(u, v), textureWindow);
                c = fetchTex<bits>(gpu, texel, rect.texpage, rect.clut);
                if (c.raw == 0x0000) continue;

                if constexpr (isBlended) {
                    c = c * ivec3(rect.color.r, rect.color.g, rect.color.b);
                }
            }

            if constexpr (isSemiTransparent) {
                if (!isTextured || c.k) {
                    c = PSXColor::blend(bg, c, transparency);
                }
            }

            c.k |= setMaskWhileDrawing;

            VRAM[y][x] = c.raw;
        }
    }
}

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

void Render::drawRectangle(gpu::GPU* gpu, const primitive::Rect& rect) {
    // clang-format off
#define FOO(BITS, SEMITRANSPARENT, BLENDED, CHECK_MASK)                                                                                   \
    else if (rect.bits == (BITS) && \
        rect.isSemiTransparent == (SEMITRANSPARENT) && \
        rect.isRawTexture == !(BLENDED) && \
        gpu->gp0_e6.checkMaskBeforeDraw == (CHECK_MASK) ) { \
        rectangle<bitsToDepth<BITS>(), SEMITRANSPARENT, BLENDED, CHECK_MASK>(gpu, rect);                                      \
    }

    if constexpr (false) {}
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ false, /* isBlended = */ false, /* checkMaskBit = */ false)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ false, /* isBlended = */ false, /* checkMaskBit = */ false)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ false, /* isBlended = */ false, /* checkMaskBit = */ false)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ false, /* isBlended = */ false, /* checkMaskBit = */ false)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ true , /* isBlended = */ false, /* checkMaskBit = */ false)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ true , /* isBlended = */ false, /* checkMaskBit = */ false)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ true , /* isBlended = */ false, /* checkMaskBit = */ false)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ true , /* isBlended = */ false, /* checkMaskBit = */ false)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ false, /* isBlended = */ true , /* checkMaskBit = */ false)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ false, /* isBlended = */ true , /* checkMaskBit = */ false)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ false, /* isBlended = */ true , /* checkMaskBit = */ false)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ false, /* isBlended = */ true , /* checkMaskBit = */ false)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ true , /* isBlended = */ true , /* checkMaskBit = */ false)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ true , /* isBlended = */ true , /* checkMaskBit = */ false)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ true , /* isBlended = */ true , /* checkMaskBit = */ false)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ true , /* isBlended = */ true , /* checkMaskBit = */ false)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ false, /* isBlended = */ false, /* checkMaskBit = */ true )
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ false, /* isBlended = */ false, /* checkMaskBit = */ true )
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ false, /* isBlended = */ false, /* checkMaskBit = */ true )
    FOO(/* bits = */ 16, /* isSemiTransparent = */ false, /* isBlended = */ false, /* checkMaskBit = */ true )
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ true , /* isBlended = */ false, /* checkMaskBit = */ true )
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ true , /* isBlended = */ false, /* checkMaskBit = */ true )
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ true , /* isBlended = */ false, /* checkMaskBit = */ true )
    FOO(/* bits = */ 16, /* isSemiTransparent = */ true , /* isBlended = */ false, /* checkMaskBit = */ true )
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ false, /* isBlended = */ true , /* checkMaskBit = */ true )
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ false, /* isBlended = */ true , /* checkMaskBit = */ true )
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ false, /* isBlended = */ true , /* checkMaskBit = */ true )
    FOO(/* bits = */ 16, /* isSemiTransparent = */ false, /* isBlended = */ true , /* checkMaskBit = */ true )
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ true , /* isBlended = */ true , /* checkMaskBit = */ true )
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ true , /* isBlended = */ true , /* checkMaskBit = */ true )
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ true , /* isBlended = */ true , /* checkMaskBit = */ true )
    FOO(/* bits = */ 16, /* isSemiTransparent = */ true , /* isBlended = */ true , /* checkMaskBit = */ true )
    // clang-format on
}