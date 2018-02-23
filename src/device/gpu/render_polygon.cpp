#include <algorithm>
#include <glm/glm.hpp>
#include "math_utils.h"
#include "psx_color.h"
#include "render.h"
#include "texture_utils.h"

#undef VRAM
#define VRAM ((uint16_t(*)[GPU::VRAM_WIDTH])gpu->vram.data())

// clang-format off
int ditherTable[4][4] = {
    {-4, +0, -3, +1}, 
    {+2, -2, +3, -1}, 
    {-3, +1, -4, +0}, 
    {+3, -1, +2, -2}
};
// clang-format on

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

    glm::ivec2 p;
    precalcBarycentric(pos);

    for (p.y = min.y; p.y < max.y; p.y++) {
        for (p.x = min.x; p.x < max.x; p.x++) {
            glm::vec3 s = barycentric(pos, p);
            // Check if point is outside triangle OR result is NAN
            if (s.x < 0 || s.y < 0 || s.z < 0 || s.x != s.x || s.y != s.y || s.z != s.z) continue;

            PSXColor c;
            if (bits == 0) {
                // clang-format off
                glm::vec3 calculatedColor = glm::vec3(
                    s.x * color[0].r + s.y * color[1].r + s.z * color[2].r,
                    s.x * color[0].g + s.y * color[1].g + s.z * color[2].g,
                    s.x * color[0].b + s.y * color[1].b + s.z * color[2].b
                );
                // clang-format on
                // if (flags & Vertex::Dithering && !(flags & Vertex::RawTexture)) {
                //     calculatedColor += ditherTable[p.y % 4][p.x % 4];
                // }
                c._ = to15bit((uint8_t)(255 * calculatedColor.r), (uint8_t)(255 * calculatedColor.g), (uint8_t)(255 * calculatedColor.b));
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
                    PSXColor raw = VRAM[texPage.y + calculatedTexel.y][texPage.x + calculatedTexel.x];
                    // TODO: I don't understand why this is necessary
                    // Without it PSOne BIOS display invalid colors under "Memory Card" and "CD Player"
                    c.r = raw.b;
                    c.g = raw.g;
                    c.b = raw.r;
                    c.k = raw.k;
                }
            }

            if ((bits != 0 || (flags & Vertex::SemiTransparency)) && c._ == 0x0000) continue;

            if (bits != 0 && !(flags & Vertex::RawTexture)) {
                c = c * color[0];
            }

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
    }
}

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
    texpage = glm::ivec2(v[0].texpage[0], v[0].texpage[1]);
    clut = glm::ivec2(v[0].clut[0], v[0].clut[1]);
    bits = v[0].bitcount;
    flags = v[0].flags;

    triangle(gpu, pos, color, texcoord, texpage, clut, bits, flags);
}