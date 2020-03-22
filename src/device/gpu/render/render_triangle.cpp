#include <algorithm>
#include <glm/glm.hpp>
#include "device/gpu/psx_color.h"
#include "dither.h"
#include "render.h"
#include "texture_utils.h"
#include "utils/macros.h"

using glm::ivec2;
using glm::ivec3;
using glm::uvec2;
using glm::vec3;
using gpu::GPU;
using gpu::Vertex;

#undef VRAM
#define VRAM ((uint16_t(*)[gpu::VRAM_WIDTH])gpu->vram.data())

int orient2d(const ivec2& a, const ivec2& b, const ivec2& c) {  //
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

vec3 getInterpolationConsts(glm::ivec2 p[3], const float w1, const float w2, const float w3) {
    float x1 = p[0].x;
    float y1 = p[0].y;
    float x2 = p[1].x;
    float y2 = p[1].y;
    float x3 = p[2].x;
    float y3 = p[2].y;

    float area = x1 * y2 - x2 * y1 + x2 * y3 - x3 * y2 + x3 * y1 - x1 * y3;

    return vec3(                                                                                 //
        ((y2 - y3) * w1 + (y3 - y1) * w2 + (y1 - y2) * w3) / area,                               //
        ((x3 - x2) * w1 + (x1 - x3) * w2 + (x2 - x1) * w3) / area,                               //
        ((x2 * y3 - x3 * y2) * w1 + (x3 * y1 - x1 * y3) * w2 + (x1 * y2 - x2 * y1) * w3) / area  //
    );
}

template <bool isGouroudShaded, bool isBlended, bool dithering>
INLINE PSXColor doShading(const ivec3 colorInterpolated, const ivec2 p, const ivec3 color[3]) {
    if constexpr (!isGouroudShaded) {
        return to15bit(color[0].r, color[0].g, color[0].b);
    }

    ivec3 outColor = colorInterpolated;

    if constexpr (dithering && isBlended) {
        outColor += ditherTable[p.y & 3u][p.x & 3u];
        outColor = glm::clamp(outColor, 0, 255);
    }

    return to15bit(outColor.r, outColor.g, outColor.b);
}

INLINE glm::uvec2 calculateTexel(glm::uvec2 texel, const gpu::GP0_E2 textureWindow) {
    // Texture is repeated outside of 256x256 window
    texel.x %= 256u;
    texel.y %= 256u;

    // Texture masking
    // texel = (texel AND(NOT(Mask * 8))) OR((Offset AND Mask) * 8)
    texel.x = (texel.x & ~(textureWindow.maskX * 8)) | ((textureWindow.offsetX & textureWindow.maskX) * 8);
    texel.y = (texel.y & ~(textureWindow.maskY * 8)) | ((textureWindow.offsetY & textureWindow.maskY) * 8);

    return texel;
}

static bool isTopLeft(const ivec2 e) { return e.y < 0 || (e.y == 0 && e.x < 0); }

template <ColorDepth bits, bool isSemiTransparent, bool isGouroudShaded, bool isBlended, bool checkMaskBeforeDraw, bool dithering>
void rasterizeTriangle(GPU* gpu, const primitive::Triangle& triangle) {
    // Extract common GPU state
    const auto transparency = triangle.transparency;
    const bool setMaskWhileDrawing = gpu->gp0_e6.setMaskWhileDrawing;
    const auto textureWindow = gpu->gp0_e2;
    constexpr bool isTextured = bits != ColorDepth::NONE;

    glm::ivec2 pos[3] = {triangle.v[0].pos, triangle.v[1].pos, triangle.v[2].pos};
    glm::vec2 uv[3] = {triangle.v[0].uv, triangle.v[1].uv, triangle.v[2].uv};

    const int area = orient2d(pos[0], pos[1], pos[2]);
    if (area == 0) return;

    for (int i = 0; i < 3; i++) {
        pos[i].x += gpu->drawingOffsetX;
        pos[i].y += gpu->drawingOffsetY;
    }

    ivec2 min(                                     //
        std::min({pos[0].x, pos[1].x, pos[2].x}),  //
        std::min({pos[0].y, pos[1].y, pos[2].y})   //
    );
    ivec2 max(                                     //
        std::max({pos[0].x, pos[1].x, pos[2].x}),  //
        std::max({pos[0].y, pos[1].y, pos[2].y})   //
    );

    // Skip rendering when distance between vertices is bigger than 1023x511
    const ivec2 size = max - min;
    if (size.x >= 1024 || size.y >= 512) return;

    min = ivec2(                  //
        gpu->minDrawingX(min.x),  //
        gpu->minDrawingY(min.y)   //
    );
    max = ivec2(                  //
        gpu->maxDrawingX(max.x),  //
        gpu->maxDrawingY(max.y)   //
    );

    // https://fgiesen.wordpress.com/2013/02/10/optimizing-the-basic-rasterizer/

    // Delta constants
    const ivec2 D01(pos[1].x - pos[0].x, pos[0].y - pos[1].y);
    const ivec2 D12(pos[2].x - pos[1].x, pos[1].y - pos[2].y);
    const ivec2 D20(pos[0].x - pos[2].x, pos[2].y - pos[0].y);

    // Fill rule
    const int bias[3] = {
        isTopLeft(D12) ? -1 : 0,  //
        isTopLeft(D20) ? -1 : 0,  //
        isTopLeft(D01) ? -1 : 0   //
    };

    // Calculate half-space values for first pixel
    int CY[3] = {
        orient2d(pos[1], pos[2], min) + bias[0],  //
        orient2d(pos[2], pos[0], min) + bias[1],  //
        orient2d(pos[0], pos[1], min) + bias[2]   //
    };

    ivec3 COLOR[3];
    for (int i = 0; i < 3; i++) {
        COLOR[i] = ivec3(triangle.v[i].color.r, triangle.v[i].color.g, triangle.v[i].color.b);
    }

    vec3 R = getInterpolationConsts(pos, triangle.v[0].color.r, triangle.v[1].color.r, triangle.v[2].color.r);
    vec3 G = getInterpolationConsts(pos, triangle.v[0].color.g, triangle.v[1].color.g, triangle.v[2].color.g);
    vec3 B = getInterpolationConsts(pos, triangle.v[0].color.b, triangle.v[1].color.b, triangle.v[2].color.b);
    vec3 U = getInterpolationConsts(pos, uv[0].x, uv[1].x, uv[2].x);
    vec3 V = getInterpolationConsts(pos, uv[0].y, uv[1].y, uv[2].y);

    ivec2 p;
    for (p.y = min.y; p.y <= max.y; p.y++) {
        ivec3 CX = ivec3(CY[0], CY[1], CY[2]);
        for (p.x = min.x; p.x <= max.x; p.x++) {
            if ((CX[0] | CX[1] | CX[2]) > 0) {
                // // Bias is subtracted to remove error from texture sampling.
                // // I don't like this, but haven't found other solution that would fix this issue.
                // ivec3 s(CX[0] - bias[0], CX[1] - bias[1], CX[2] - bias[2]);

                PSXColor bg = VRAM[p.y][p.x];
                if constexpr (checkMaskBeforeDraw) {
                    if (bg.k) goto DONE;
                }

                ivec3 colorInterpolated = ivec3(R.x * p.x + R.y * p.y + R.z,  //
                                                G.x * p.x + G.y * p.y + G.z,  //
                                                B.x * p.x + B.y * p.y + B.z   //
                );

                PSXColor c;
                if constexpr (bits == ColorDepth::NONE) {
                    c = doShading<isGouroudShaded, isBlended, dithering>(colorInterpolated, p, COLOR);
                } else {
                    auto UV = ivec2(                  //
                        U.x * p.x + U.y * p.y + U.z,  //
                        V.x * p.x + V.y * p.y + V.z   //
                    );
                    uvec2 texel = calculateTexel(UV, textureWindow);
                    c = fetchTex<bits>(gpu, texel, triangle.texpage, triangle.clut);
                    if (c.raw == 0x0000) goto DONE;

                    if constexpr (isBlended) {
                        if constexpr (isGouroudShaded) {
                            c = c * colorInterpolated;
                        } else {  // Flat shading
                            c = c * COLOR[0];
                        }
                    }
                    // TODO: Textured polygons are not dithered
                }

                if constexpr (isSemiTransparent) {
                    if (!isTextured || c.k) {
                        c = PSXColor::blend(bg, c, transparency);
                    }
                }

                c.k |= setMaskWhileDrawing;

                VRAM[p.y][p.x] = c.raw;

                //////////////////////////////////////////////////////////////////////
            }

        DONE:
            CX[0] += D12.y;
            CX[1] += D20.y;
            CX[2] += D01.y;
        }
        CY[0] += D12.x;
        CY[1] += D20.x;
        CY[2] += D01.x;
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

void Render::drawTriangle(gpu::GPU* gpu, const primitive::Triangle& triangle) {
// clang-format off
#define FOO(BITS, SEMITRANSPARENT, GOUROUD, BLENDED, CHECK_MASK, DITHERING)                                                                                   \
    else if (triangle.bits == (BITS) && \
        triangle.isSemiTransparent == (SEMITRANSPARENT) && \
        triangle.gouroudShading == (GOUROUD) && \
        triangle.isRawTexture == !(BLENDED) && \
        gpu->gp0_e6.checkMaskBeforeDraw == (CHECK_MASK) && \
        gpu->gp0_e1.dither24to15 == (DITHERING) ) { \
        rasterizeTriangle<bitsToDepth<BITS>(), SEMITRANSPARENT, GOUROUD, BLENDED, CHECK_MASK, DITHERING>(gpu, triangle);                                      \
    }
    if constexpr (false) {}
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ false)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ false)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ false, /* dithering = */ true)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ false, /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ false, /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ true , /* isGouroudShaded = */ false, /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ false, /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 0,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 4,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 8,  /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ true)
    FOO(/* bits = */ 16, /* isSemiTransparent = */ true , /* isGouroudShaded = */ true , /* isBlended = */ true , /* checkMaskBit = */ true , /* dithering = */ true)
    // clang-format on
}