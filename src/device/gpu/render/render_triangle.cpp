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
using gpu::GPU;
using gpu::Vertex;

#undef VRAM
#define VRAM ((uint16_t(*)[gpu::VRAM_WIDTH])gpu->vram.data())

int orient2d(const ivec2& a, const ivec2& b, const ivec2& c) {  //
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

INLINE PSXColor doShading(const ivec3 s, const int area, const ivec2 p, const ivec3 color[3], const primitive::Triangle& triangle,
                          const bool dithered) {
    if (!triangle.gouroudShading) {
        return to15bit(color[0].r, color[0].g, color[0].b);
    }

    ivec3 outColor(                                                //
        (s.x * color[0].r + s.y * color[1].r + s.z * color[2].r),  //
        (s.x * color[0].g + s.y * color[1].g + s.z * color[2].g),  //
        (s.x * color[0].b + s.y * color[1].b + s.z * color[2].b)   //
    );

    outColor /= area;

    if (dithered && !triangle.isRawTexture) {
        outColor += ditherTable[p.y & 3u][p.x & 3u];
        outColor = glm::clamp(outColor, 0, 255);
    }

    return to15bit(outColor.r, outColor.g, outColor.b);
}

INLINE glm::uvec2 calculateTexel(const glm::ivec3 s, const int area, const glm::ivec2 tex[3], const gpu::GP0_E2 textureWindow) {
    glm::uvec2 texel(                                                                   //
        ((int64_t)s.x * tex[0].x + (int64_t)s.y * tex[1].x + (int64_t)s.z * tex[2].x),  //
        ((int64_t)s.x * tex[0].y + (int64_t)s.y * tex[1].y + (int64_t)s.z * tex[2].y)   //
    );

    texel /= area;

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

template <ColorDepth bits>
void rasterizeTriangle(GPU* gpu, const primitive::Triangle& triangle) {
    // Extract common GPU state
    const auto transparency = triangle.transparency;
    const bool checkMaskBeforeDraw = gpu->gp0_e6.checkMaskBeforeDraw;
    const bool setMaskWhileDrawing = gpu->gp0_e6.setMaskWhileDrawing;
    const auto textureWindow = gpu->gp0_e2;
    const bool dithering = gpu->gp0_e1.dither24to15;
    const bool isBlended = !triangle.isRawTexture;
    const bool isGouroudShaded = triangle.gouroudShading;
    const bool isSemiTransparent = triangle.isSemiTransparent;
    constexpr bool isTextured = bits != ColorDepth::NONE;

    glm::ivec2 pos[3] = {triangle.v[0].pos, triangle.v[1].pos, triangle.v[2].pos};
    glm::ivec2 uv[3] = {triangle.v[0].uv, triangle.v[1].uv, triangle.v[2].uv};

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

    // Skip rendering when distence between vertices is bigger than 1023x511
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

    ivec2 p;
    for (p.y = min.y; p.y <= max.y; p.y++) {
        ivec3 CX = ivec3(CY[0], CY[1], CY[2]);
        for (p.x = min.x; p.x <= max.x; p.x++) {
            if ((CX[0] | CX[1] | CX[2]) > 0) {
                // Bias is subtracted to remove error from texture sampling.
                // I don't like this, but haven't found other solution that would fix this issue.
                ivec3 s(CX[0] - bias[0], CX[1] - bias[1], CX[2] - bias[2]);

                PSXColor bg = VRAM[p.y][p.x];
                if (unlikely(checkMaskBeforeDraw)) {
                    if (bg.k) goto DONE;
                }

                PSXColor c;
                if constexpr (bits == ColorDepth::NONE) {
                    c = doShading(s, area, p, COLOR, triangle, dithering);
                } else {
                    uvec2 texel = calculateTexel(s, area, uv, textureWindow);
                    c = fetchTex<bits>(gpu, texel, triangle.texpage, triangle.clut);
                    if (c.raw == 0x0000) goto DONE;

                    if (isBlended) {
                        ivec3 brightness;
                        if (isGouroudShaded) {
                            brightness = ivec3(                                            //
                                (s.x * COLOR[0].r + s.y * COLOR[1].r + s.z * COLOR[2].r),  //
                                (s.x * COLOR[0].g + s.y * COLOR[1].g + s.z * COLOR[2].g),  //
                                (s.x * COLOR[0].b + s.y * COLOR[1].b + s.z * COLOR[2].b)   //
                            );
                            brightness /= area;
                        } else {  // Flat shading
                            brightness = COLOR[0];
                        }

                        c = c * brightness;
                    }
                    // TODO: Textured polygons are not dithered
                }

                if (isSemiTransparent && (!isTextured || c.k)) {
                    c = PSXColor::blend(bg, c, transparency);
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

void Render::drawTriangle(gpu::GPU* gpu, const primitive::Triangle& triangle) {
    if (triangle.bits == 0) {
        rasterizeTriangle<ColorDepth::NONE>(gpu, triangle);
    } else if (triangle.bits == 4) {
        rasterizeTriangle<ColorDepth::BIT_4>(gpu, triangle);
    } else if (triangle.bits == 8) {
        rasterizeTriangle<ColorDepth::BIT_8>(gpu, triangle);
    } else if (triangle.bits == 16) {
        rasterizeTriangle<ColorDepth::BIT_16>(gpu, triangle);
    }
}