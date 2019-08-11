#include <algorithm>
#include <glm/glm.hpp>
#include "device/gpu/psx_color.h"
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

const int8_t ditherTable[4][4] = {
    {-4, +0, -3, +1},  //
    {+2, -2, +3, -1},  //
    {-3, +1, -4, +0},  //
    {+3, -1, +2, -2}   //
};

int orient2d(const ivec2& a, const ivec2& b, const ivec2& c) {  //
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool isCw(const ivec2 v[3]) {
    vec3 a = vec3(v[0].x, v[0].y, 1);
    vec3 b = vec3(v[1].x, v[1].y, 1);
    vec3 c = vec3(v[2].x, v[2].y, 1);

    vec3 ab = b - a;
    vec3 ac = c - a;
    vec3 n = glm::cross(ab, ac);

    return n.z < 0;
}

INLINE uvec2 calculateTexel(const ivec3 s, const int area, const ivec2 tex[3], const gpu::GP0_E2 textureWindow) {
    uvec2 texel(                                                                               //
        ((int64_t)s.x * tex[0].x + (int64_t)s.y * tex[1].x + (int64_t)s.z * tex[2].x) / area,  //
        ((int64_t)s.x * tex[0].y + (int64_t)s.y * tex[1].y + (int64_t)s.z * tex[2].y) / area   //
    );

    // Texture is repeated outside of 256x256 window
    texel.x %= 256u;
    texel.y %= 256u;

    // Texture masking
    // texel = (texel AND(NOT(Mask * 8))) OR((Offset AND Mask) * 8)
    texel.x = (texel.x & ~(textureWindow.maskX * 8)) | ((textureWindow.offsetX & textureWindow.maskX) * 8);
    texel.y = (texel.y & ~(textureWindow.maskY * 8)) | ((textureWindow.offsetY & textureWindow.maskY) * 8);

    return texel;
}

INLINE PSXColor doShading(const ivec3 s, const int area, const ivec2 p, const ivec3 color[3], const int flags) {
    ivec3 outColor(                                                       //
        (s.x * color[0].r + s.y * color[1].r + s.z * color[2].r) / area,  //
        (s.x * color[0].g + s.y * color[1].g + s.z * color[2].g) / area,  //
        (s.x * color[0].b + s.y * color[1].b + s.z * color[2].b) / area   //
    );

    // TODO: THPS2 fading screen doesn't look as it should
    if (flags & Vertex::Dithering && !(flags & Vertex::RawTexture)) {
        outColor += ditherTable[p.y % 4u][p.x % 4u];
        outColor = glm::clamp(outColor, 0, 255);
    }

    return to15bit(outColor.r, outColor.g, outColor.b);
}

template <ColorDepth bits>
INLINE PSXColor fetchTex(GPU* gpu, uvec2 texel, const ivec2 texPage, const ivec2 clut) {
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

template <ColorDepth bits>
INLINE void plotPixel(GPU* gpu, const ivec2 p, const ivec3 s, const int area, const ivec3 color[3], const ivec2 tex[3], const ivec2 texPage,
                      const ivec2 clut, const int flags, const gpu::GP0_E2 textureWindow, const gpu::GP0_E6 maskSettings) {
    if (unlikely(maskSettings.checkMaskBeforeDraw)) {
        PSXColor bg = VRAM[p.y][p.x];
        if (bg.k) return;
    }

    constexpr bool isTextured = bits != ColorDepth::NONE;
    const bool isSemiTransparent = flags & Vertex::SemiTransparency;
    const bool isBlended = !(flags & Vertex::RawTexture);

    PSXColor c;
    if constexpr (bits == ColorDepth::NONE) {
        c = doShading(s, area, p, color, flags);
    } else {
        uvec2 texel = calculateTexel(s, area, tex, textureWindow);
        c = fetchTex<bits>(gpu, texel, texPage, clut);
        if (c.raw == 0x0000) return;

        if (isBlended) {
            vec3 brightness;

            if (flags & Vertex::GouroudShading) {
                // TODO: Get rid of float colors
                vec3 fcolor[3];
                fcolor[0] = vec3(color[0]) / 255.f;
                fcolor[1] = vec3(color[1]) / 255.f;
                fcolor[2] = vec3(color[2]) / 255.f;
                brightness = vec3(                                                       //
                    (s.x * fcolor[0].r + s.y * fcolor[1].r + s.z * fcolor[2].r) / area,  //
                    (s.x * fcolor[0].g + s.y * fcolor[1].g + s.z * fcolor[2].g) / area,  //
                    (s.x * fcolor[0].b + s.y * fcolor[1].b + s.z * fcolor[2].b) / area);
            } else {  // Flat shading
                brightness = vec3(color[0]) / 255.f;
            }

            c = c * (brightness * 2.f);
        }
    }

    if (isSemiTransparent && (!isTextured || c.k)) {
        using Transparency = gpu::GP0_E1::SemiTransparency;

        auto transparency = (Transparency)((flags & 0x60) >> 5);
        PSXColor bg = VRAM[p.y][p.x];
        switch (transparency) {
            case Transparency::Bby2plusFby2: c = bg / 2.f + c / 2.f; break;
            case Transparency::BplusF: c = bg + c; break;
            case Transparency::BminusF: c = bg - c; break;
            case Transparency::BplusFby4: c = bg + c / 4.f; break;
        }
    }

    c.k |= maskSettings.setMaskWhileDrawing;

    VRAM[p.y][p.x] = c.raw;
}

template <ColorDepth bits>
INLINE void triangle(GPU* gpu, const ivec2 pos[3], const ivec3 color[3], const ivec2 tex[3], const ivec2 texPage, const ivec2 clut,
                     const int flags, const gpu::GP0_E2 textureWindow, const gpu::GP0_E6 maskSettings) {
    const ivec2 min = ivec2(                                         //
        gpu->minDrawingX(std::min({pos[0].x, pos[1].x, pos[2].x})),  //
        gpu->minDrawingY(std::min({pos[0].y, pos[1].y, pos[2].y}))   //
    );
    const ivec2 max = ivec2(                                         //
        gpu->maxDrawingX(std::max({pos[0].x, pos[1].x, pos[2].x})),  //
        gpu->maxDrawingY(std::max({pos[0].y, pos[1].y, pos[2].y}))   //
    );

    // https://fgiesen.wordpress.com/2013/02/10/optimizing-the-basic-rasterizer/
    const int A01 = pos[0].y - pos[1].y;
    const int B01 = pos[1].x - pos[0].x;

    const int A12 = pos[1].y - pos[2].y;
    const int B12 = pos[2].x - pos[1].x;

    const int A20 = pos[2].y - pos[0].y;
    const int B20 = pos[0].x - pos[2].x;

    const ivec2 minp = ivec2(min.x, min.y);
    int w0_row = orient2d(pos[1], pos[2], minp);
    int w1_row = orient2d(pos[2], pos[0], minp);
    int w2_row = orient2d(pos[0], pos[1], minp);

    const int area = orient2d(pos[0], pos[1], pos[2]);
    if (area == 0) return;

    ivec2 p;

    for (p.y = min.y; p.y < max.y; p.y++) {
        ivec3 is = ivec3(w0_row, w1_row, w2_row);
        for (p.x = min.x; p.x < max.x; p.x++) {
            if ((is.x | is.y | is.z) >= 0) {
                plotPixel<bits>(gpu, p, is, area, color, tex, texPage, clut, flags, textureWindow, maskSettings);
            }

            is.x += A12;
            is.y += A20;
            is.z += A01;
        }
        w0_row += B12;
        w1_row += B20;
        w2_row += B01;
    }
}

// TODO: Render in batches
void Render::drawTriangle(GPU* gpu, Vertex v[3]) {
    ivec2 pos[3];
    ivec3 color[3];
    ivec2 texcoord[3];
    ivec2 texpage;
    ivec2 clut;
    int bits;
    int flags;
    gpu::GP0_E2 textureWindow;
    gpu::GP0_E6 maskSettings;
    for (int j = 0; j < 3; j++) {
        pos[j] = ivec2(v[j].position[0], v[j].position[1]);
        color[j] = ivec3(v[j].color[0], v[j].color[1], v[j].color[2]);
        texcoord[j] = ivec2(v[j].texcoord[0], v[j].texcoord[1]);
    }

    // TODO: Remove this hack
    if (isCw(pos)) {
        std::swap(pos[1], pos[2]);
        std::swap(color[1], color[2]);
        std::swap(texcoord[1], texcoord[2]);
    }
    texpage = ivec2(v[0].texpage[0], v[0].texpage[1]);
    clut = ivec2(v[0].clut[0], v[0].clut[1]);
    bits = v[0].bitcount;
    flags = v[0].flags;
    textureWindow = v[0].textureWindow;
    maskSettings = v[0].maskSettings;

    // Skip rendering when distence between vertices is bigger than 1023x511
    for (int j = 0; j < 3; j++) {
        if (abs(pos[j].x - pos[(j + 1) % 3].x) >= 1024) return;
        if (abs(pos[j].y - pos[(j + 1) % 3].y) >= 512) return;
    }

    if (bits == 0) triangle<ColorDepth::NONE>(gpu, pos, color, texcoord, texpage, clut, flags, textureWindow, maskSettings);
    if (bits == 4) triangle<ColorDepth::BIT_4>(gpu, pos, color, texcoord, texpage, clut, flags, textureWindow, maskSettings);
    if (bits == 8) triangle<ColorDepth::BIT_8>(gpu, pos, color, texcoord, texpage, clut, flags, textureWindow, maskSettings);
    if (bits == 16) triangle<ColorDepth::BIT_16>(gpu, pos, color, texcoord, texpage, clut, flags, textureWindow, maskSettings);
}