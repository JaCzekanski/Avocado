#include <algorithm>
#include <glm/glm.hpp>
#include "gpu.h"
#include "math_utils.h"

glm::vec2 v0, v1;
float d00, d01, d11, denom;

void precalcBarycentric(glm::ivec2 pos[3]) {
    v0 = pos[1] - pos[0];
    v1 = pos[2] - pos[0];
    d00 = glm::dot(v0, v0);
    d01 = glm::dot(v0, v1);
    d11 = glm::dot(v1, v1);
    denom = d00 * d11 - d01 * d01;
}

glm::vec3 barycentric(glm::ivec2 pos[3], glm::ivec2 p) {
    glm::vec2 v2 = p - pos[0];
    float d20 = glm::dot(v2, v0);
    float d21 = glm::dot(v2, v1);
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;

    return glm::vec3(u, v, w);
}

inline uint16_t GPU::tex4bit(glm::ivec2 tex, glm::ivec2 texPage, glm::ivec2 clut) {
    uint16_t index = VRAM[texPage.y + tex.y][texPage.x + tex.x / 4];
    uint16_t entry = (index >> ((tex.x & 3) * 4)) & 0xf;
    return VRAM[clut.y][clut.x + entry];
}

inline uint16_t GPU::tex8bit(glm::ivec2 tex, glm::ivec2 texPage, glm::ivec2 clut) {
    uint16_t index = VRAM[texPage.y + tex.y][texPage.x + tex.x / 2];
    uint16_t entry = (index >> ((tex.x & 1) * 8)) & 0xff;
    return VRAM[clut.y][clut.x + entry];
}

union PSXColor {
    struct {
        uint16_t r : 5;
        uint16_t g : 5;
        uint16_t b : 5;
        uint16_t k : 1;
    };
    uint16_t _;

    PSXColor() : _(0) {}
    PSXColor(uint16_t color) : _(color) {}

    PSXColor operator+(PSXColor rhs) {
        r = clamp<int>(r + rhs.r, 0, 31);
        g = clamp<int>(g + rhs.g, 0, 31);
        b = clamp<int>(b + rhs.b, 0, 31);
        return *this;
    }

    PSXColor operator-(PSXColor rhs) {
        r = clamp<int>(r - rhs.r, 0, 31);
        g = clamp<int>(g - rhs.g, 0, 31);
        b = clamp<int>(b - rhs.b, 0, 31);
        return *this;
    }

    PSXColor operator*(float rhs) {
        r = (uint16_t)(r * rhs);
        g = (uint16_t)(g * rhs);
        b = (uint16_t)(b * rhs);
        return *this;
    }

    PSXColor operator/(float rhs) {
        r = (uint16_t)(r / rhs);
        g = (uint16_t)(g / rhs);
        b = (uint16_t)(b / rhs);
        return *this;
    }

    PSXColor operator*(glm::vec3 rhs) {
        r = clamp<uint16_t>((uint16_t)(rhs.r / 0.5f * r), 0, 31);
        g = clamp<uint16_t>((uint16_t)(rhs.g / 0.5f * g), 0, 31);
        b = clamp<uint16_t>((uint16_t)(rhs.b / 0.5f * b), 0, 31);
        return *this;
    }
};

int ditherTable[4][4] = {{-4, +0, -3, +1}, {+2, -2, +3, -1}, {-3, +1, -4, +0}, {+3, -1, +2, -2}};

void GPU::triangle(glm::ivec2 pos[3], glm::vec3 color[3], glm::ivec2 tex[3], glm::ivec2 texPage, glm::ivec2 clut, int bits, int flags) {
    for (int i = 0; i < 3; i++) {
        pos[i].x += drawingOffsetX;
        pos[i].y += drawingOffsetY;
    }
    // clang-format off
    glm::ivec2 min = glm::ivec2(
        minDrawingX(std::min({ pos[0].x, pos[1].x, pos[2].x })),
        minDrawingY(std::min({ pos[0].y, pos[1].y, pos[2].y }))
    );
    glm::ivec2 max = glm::ivec2(
        maxDrawingX(std::max({ pos[0].x, pos[1].x, pos[2].x })),
        maxDrawingY(std::max({ pos[0].y, pos[1].y, pos[2].y }))
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
                    c = tex4bit(calculatedTexel, texPage, clut);
                } else if (bits == 8) {
                    c = tex8bit(calculatedTexel, texPage, clut);
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
                GP0_E1::SemiTransparency transparency = (GP0_E1::SemiTransparency)((flags & 0xA0) >> 6);
                PSXColor bg = VRAM[p.y][p.x];
                if (transparency == GP0_E1::SemiTransparency::Bby2plusFby2) {
                    c = bg / 2.f + c / 2.f;
                } else if (transparency == GP0_E1::SemiTransparency::BplusF) {
                    c = bg + c;
                } else if (transparency == GP0_E1::SemiTransparency::BminusF) {
                    c = bg - c;
                } else if (transparency == GP0_E1::SemiTransparency::BplusFby4) {
                    c = bg + c / 4.f;
                }
            }

            VRAM[p.y][p.x] = c._;
        }
    }
}

void GPU::drawTriangle(Vertex v[3]) {
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

    triangle(pos, color, texcoord, texpage, clut, bits, flags);
}