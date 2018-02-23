#include <algorithm>
#include <glm/glm.hpp>
#include "psx_color.h"
#include "render.h"
#include "texture_utils.h"

#undef VRAM
#define VRAM ((uint16_t(*)[VRAM_WIDTH])gpu->vram.data())

// clang-format off
int ditherTable[4][4] = {
    {-4, +0, -3, +1}, 
    {+2, -2, +3, -1}, 
    {-3, +1, -4, +0}, 
    {+3, -1, +2, -2}
};
// clang-format on

int orient2d(const glm::ivec2& a, const glm::ivec2& b, const glm::ivec2& c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool isCw(const glm::ivec2 v[3]) {
    glm::vec3 a = glm::vec3(v[0].x, v[0].y, 1);
    glm::vec3 b = glm::vec3(v[1].x, v[1].y, 1);
    glm::vec3 c = glm::vec3(v[2].x, v[2].y, 1);

    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 n = glm::cross(ab, ac);

    return n.z < 0;
}

void triangle(GPU* gpu, glm::ivec2 pos[3], glm::vec3 color[3], glm::ivec2 tex[3], glm::ivec2 texPage, glm::ivec2 clut, int bits,
              int flags) {
    for (int i = 0; i < 3; i++) {
        pos[i].x += gpu->drawingOffsetX;
        pos[i].y += gpu->drawingOffsetY;
    }
    // clang-format off
    glm::ivec2 min = glm::ivec2(
        gpu->minDrawingX(std::min({ pos[0].x, pos[1].x, pos[2].x })),
        gpu->minDrawingY(std::min({ pos[0].y, pos[1].y, pos[2].y }))
    );
    glm::ivec2 max = glm::ivec2(
        gpu->maxDrawingX(std::max({ pos[0].x, pos[1].x, pos[2].x })),
        gpu->maxDrawingY(std::max({ pos[0].y, pos[1].y, pos[2].y }))
    );
    // clang-format on

    // https://fgiesen.wordpress.com/2013/02/10/optimizing-the-basic-rasterizer/
    int A01 = pos[0].y - pos[1].y;
    int B01 = pos[1].x - pos[0].x;

    int A12 = pos[1].y - pos[2].y;
    int B12 = pos[2].x - pos[1].x;

    int A20 = pos[2].y - pos[0].y;
    int B20 = pos[0].x - pos[2].x;

    glm::ivec2 minp = glm::ivec2(min.x, min.y);
    int w0_row = orient2d(pos[1], pos[2], minp);
    int w1_row = orient2d(pos[2], pos[0], minp);
    int w2_row = orient2d(pos[0], pos[1], minp);

    int area = orient2d(pos[0], pos[1], pos[2]);
    if (area == 0) return;

    glm::ivec2 p;

    for (p.y = min.y; p.y < max.y; p.y++) {
        glm::ivec3 is = glm::ivec3(w0_row, w1_row, w2_row);
        for (p.x = min.x; p.x < max.x; p.x++) {
            if ((is.x | is.y | is.z) >= 0) {
                glm::vec3 s = glm::vec3(is) / (float)area;

                PSXColor c;
                if (bits == 0) {
                    // clang-format off
                    glm::vec3 calculatedColor = glm::vec3(
                        s.x * color[0].r + s.y * color[1].r + s.z * color[2].r,
                        s.x * color[0].g + s.y * color[1].g + s.z * color[2].g,
                        s.x * color[0].b + s.y * color[1].b + s.z * color[2].b
                    );
                    // clang-format on

                    // TODO: THPS2 fading screen doesn't look as it should
                    if (flags & Vertex::Dithering && !(flags & Vertex::RawTexture)) {
                        calculatedColor += ditherTable[p.y % 4][p.x % 4] / 255.f;
                        calculatedColor = glm::clamp(calculatedColor, 0.f, 1.f);
                    }
                    c._ = to15bit((uint8_t)(255 * calculatedColor.r), (uint8_t)(255 * calculatedColor.g),
                                  (uint8_t)(255 * calculatedColor.b));
                } else {
                    // clang-format off
                    glm::ivec2 calculatedTexel = glm::ivec2(
                        roundf(s.x * tex[0].x + s.y * tex[1].x + s.z * tex[2].x),
                        roundf(s.x * tex[0].y + s.y * tex[1].y + s.z * tex[2].y)
                    );
                    // clang-format on
                    if (bits == 4) {
                        c = tex4bit(gpu, calculatedTexel, texPage, clut);
                    } else if (bits == 8) {
                        c = tex8bit(gpu, calculatedTexel, texPage, clut);
                    } else if (bits == 16) {
                        c = VRAM[texPage.y + calculatedTexel.y][texPage.x + calculatedTexel.x];
                        // TODO: In PSOne BIOS colors are swapped (r == b, g == g, b == r, k == k)
                    }
                }

                if ((bits != 0 || (flags & Vertex::SemiTransparency)) && c._ == 0x0000) goto skip_pixel;
                if (bits != 0 && !(flags & Vertex::RawTexture)) c = c * color[0];

                if (flags & Vertex::SemiTransparency && c.k) {
                    using Transparency = GP0_E1::SemiTransparency;

                    PSXColor bg = VRAM[p.y][p.x];
                    Transparency transparency = (Transparency)((flags & 0xA0) >> 6);
                    switch (transparency) {
                        case Transparency::Bby2plusFby2:
                            c = bg / 2.f + c / 2.f;
                            break;
                        case Transparency::BplusF:
                            c = bg + c;
                            break;
                        case Transparency::BminusF:
                            c = bg - c;
                            break;
                        case Transparency::BplusFby4:
                            c = bg + c / 4.f;
                            break;
                    }
                }

                VRAM[p.y][p.x] = c._;
            }

        skip_pixel:
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
void drawTriangle(GPU* gpu, Vertex v[3]) {
    glm::ivec2 pos[3];
    glm::vec3 color[3];
    glm::ivec2 texcoord[3];
    glm::ivec2 texpage;
    glm::ivec2 clut;
    int bits;
    int flags;
    for (int j = 0; j < 3; j++) {
        pos[j] = glm::ivec2(v[j].position[0], v[j].position[1]);
        color[j] = glm::vec3(v[j].color[0] / 255.f, v[j].color[1] / 255.f, v[j].color[2] / 255.f);
        texcoord[j] = glm::ivec2(v[j].texcoord[0], v[j].texcoord[1]);
    }

    // TODO: Remove this hack
    if (isCw(pos)) {
        std::swap(pos[1], pos[2]);
        std::swap(color[1], color[2]);
        std::swap(texcoord[1], texcoord[2]);
    }
    texpage = glm::ivec2(v[0].texpage[0], v[0].texpage[1]);
    clut = glm::ivec2(v[0].clut[0], v[0].clut[1]);
    bits = v[0].bitcount;
    flags = v[0].flags;

    triangle(gpu, pos, color, texcoord, texpage, clut, bits, flags);
}