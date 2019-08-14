#include "../primitive.h"
#include "render.h"
#include "texture_utils.h"
#include "utils/macros.h"

using glm::ivec2;
using glm::uvec2;
using glm::vec3;
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

template <ColorDepth bits>
INLINE void rectangle(GPU* gpu, const primitive::Rect& rect) {
    // Extract common GPU state
    using Transparency = gpu::GP0_E1::SemiTransparency;
    const auto transparency = gpu->gp0_e1.semiTransparency;
    const bool checkMaskBeforeDraw = gpu->gp0_e6.checkMaskBeforeDraw;
    const bool setMaskWhileDrawing = gpu->gp0_e6.setMaskWhileDrawing;
    const auto textureWindow = gpu->gp0_e2;
    const bool isBlended = !rect.isRawTexture;
    constexpr bool isTextured = bits != ColorDepth::NONE;

    const ivec2 pos(                       //
        rect.pos.x + gpu->drawingOffsetX,  //
        rect.pos.y + gpu->drawingOffsetY   //
    );
    const ivec2 min(              //
        gpu->minDrawingX(pos.x),  //
        gpu->minDrawingY(pos.y)   //
    );
    const ivec2 max(                            //
        gpu->maxDrawingX(pos.x + rect.size.x),  //
        gpu->maxDrawingY(pos.y + rect.size.y)   //
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
    for (y = min.y, v = uv.y; y < max.y; y++, v += vStep) {
        for (x = min.x, u = uv.x; x < max.x; x++, u += uStep) {
            if (unlikely(checkMaskBeforeDraw)) {
                PSXColor bg = VRAM[y][x];
                if (bg.k) continue;
            }

            PSXColor c;
            if constexpr (bits == ColorDepth::NONE) {
                c = PSXColor(rect.color.r, rect.color.g, rect.color.b);
            } else {
                uvec2 texel = calculateTexel(ivec2(u, v), textureWindow);
                c = fetchTex<bits>(gpu, texel, rect.texpage, rect.clut);
                if (c.raw == 0x0000) continue;

                if (isBlended) {
                    vec3 brightness = vec3(rect.color) / 255.f;
                    c = c * (brightness * 2.f);
                }
            }

            if (rect.isSemiTransparent && (!isTextured || c.k)) {
                PSXColor bg = VRAM[y][x];
                switch (transparency) {
                    case Transparency::Bby2plusFby2: c = bg / 2.f + c / 2.f; break;
                    case Transparency::BplusF: c = bg + c; break;
                    case Transparency::BminusF: c = bg - c; break;
                    case Transparency::BplusFby4: c = bg + c / 4.f; break;
                }
            }

            c.k |= setMaskWhileDrawing;

            VRAM[y][x] = c.raw;
        }
    }
}
void Render::drawRectangle(gpu::GPU* gpu, const primitive::Rect& rect) {
    if (rect.bits == 0) {
        rectangle<ColorDepth::NONE>(gpu, rect);
    } else if (rect.bits == 4) {
        rectangle<ColorDepth::BIT_4>(gpu, rect);
    } else if (rect.bits == 8) {
        rectangle<ColorDepth::BIT_8>(gpu, rect);
    } else if (rect.bits == 16) {
        rectangle<ColorDepth::BIT_16>(gpu, rect);
    }
}