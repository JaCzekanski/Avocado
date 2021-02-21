#include "../primitive.h"
#include "render.h"
#include "texture_utils.h"
#include "utils/macros.h"

#undef VRAM
#define VRAM ((uint16_t(*)[gpu::VRAM_WIDTH])gpu->vram.data())

template <ColorDepth bits, bool isSemiTransparent, bool isBlended, bool checkMaskBeforeDraw>
INLINE void rasterizeRectangle(gpu::GPU* gpu, const primitive::Rect& rect) {
    // Extract common GPU state
    const auto transparency = gpu->gp0_e1.semiTransparency;
    const bool setMaskWhileDrawing = gpu->gp0_e6.setMaskWhileDrawing;
    const auto textureWindow = gpu->gp0_e2;
    constexpr bool isTextured = bits != ColorDepth::NONE;

    if (rect.size.x >= 1024 || rect.size.y >= 512) return;

    const ivec2 pos(  //
        rect.pos.x,   //
        rect.pos.y    //
    );
    const ivec2 min(              //
        gpu->minDrawingX(pos.x),  //
        gpu->minDrawingY(pos.y)   //
    );
    const ivec2 max(                                //
        gpu->maxDrawingX(pos.x + rect.size.x - 1),  //
        gpu->maxDrawingY(pos.y + rect.size.y - 1)   //
    );

    ivec2 uv(                         //
        rect.uv.x + (min.x - pos.x),  // Add offset if part of rectange was cut off
        rect.uv.y + (min.y - pos.y)   //
    );
    int uStep = 1, vStep = 1;

    // Texture flipping
    if (gpu->gp0_e1.texturedRectangleXFlip) {
        uv.x += 1;
        uStep = -1;
    }
    if (gpu->gp0_e1.texturedRectangleYFlip) {
        vStep = -1;
    }

    loadClutCacheIfRequired<bits>(gpu, rect.clut);

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
                const ivec2 texel = maskTexel(ivec2(u, v), textureWindow);
                c = fetchTex<bits>(gpu, texel, rect.texpage);
                if (c.raw == 0x0000) continue;

                if constexpr (isBlended) {
                    c = c * rect.color;
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

// Generate all permutations of rasterizeRectangle
using rasterizeRectangle_t = void(gpu::GPU* gpu, const primitive::Rect& rect);

#define E(bits, isSemiTransparent, isBlended, checkMaskBit) \
    &rasterizeRectangle<bitsToDepth<bits>(), isSemiTransparent, isBlended, checkMaskBit>

/* Kotlin script for lookup array generation:

    fun Iterable<Int>.wrap(f: (Int) -> String): String =
        "{" + joinToString(",", transform = f) + "}"

    fun generateTable(): String =
        setOf(0, 4, 8, 16).wrap { bits ->
        (0..1).wrap { isSemiTransparent ->
        (0..1).wrap { isBlended ->
        (0..1).wrap { checkMaskBit ->
            "E($bits, $isSemiTransparent, $isGouraudShaded, $isBlended, $checkMaskBit, $dithering)"
        }}}}

    generateTable()
*/

static constexpr rasterizeRectangle_t* rasterizeRectangleDispatchTable[4][2][2][2] =  //
    {{{{E(0, 0, 0, 0), E(0, 0, 0, 1)}, {E(0, 0, 1, 0), E(0, 0, 1, 1)}}, {{E(0, 1, 0, 0), E(0, 1, 0, 1)}, {E(0, 1, 1, 0), E(0, 1, 1, 1)}}},
     {{{E(4, 0, 0, 0), E(4, 0, 0, 1)}, {E(4, 0, 1, 0), E(4, 0, 1, 1)}}, {{E(4, 1, 0, 0), E(4, 1, 0, 1)}, {E(4, 1, 1, 0), E(4, 1, 1, 1)}}},
     {{{E(8, 0, 0, 0), E(8, 0, 0, 1)}, {E(8, 0, 1, 0), E(8, 0, 1, 1)}}, {{E(8, 1, 0, 0), E(8, 1, 0, 1)}, {E(8, 1, 1, 0), E(8, 1, 1, 1)}}},
     {{{E(16, 0, 0, 0), E(16, 0, 0, 1)}, {E(16, 0, 1, 0), E(16, 0, 1, 1)}},
      {{E(16, 1, 0, 0), E(16, 1, 0, 1)}, {E(16, 1, 1, 0), E(16, 1, 1, 1)}}}};
#undef E

void Render::drawRectangle(gpu::GPU* gpu, const primitive::Rect& rect) {
    auto bits = (int)bitsToDepth(rect.bits);
    auto isSemiTransparent = rect.isSemiTransparent;
    auto isBlended = !rect.isRawTexture;
    auto checkMaskBit = gpu->gp0_e6.checkMaskBeforeDraw;

    auto rasterize = rasterizeRectangleDispatchTable[bits][isSemiTransparent][isBlended][checkMaskBit];

    rasterize(gpu, rect);
}