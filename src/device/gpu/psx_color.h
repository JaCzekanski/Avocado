#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include "semi_transparency.h"
#include "utils/macros.h"
#include "utils/math.h"

#undef RGB  // Some header is adding this macro on Windows
// Union for storing 24bit color (used in GPU commands)
union RGB {
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t _;
    };
    uint32_t raw;

    RGB() : raw(0) {}
};

RGB operator*(const RGB& lhs, const float rhs);
RGB operator+(const RGB& lhs, const RGB& rhs);

// Union for storing 15bit color + mask bit (native VRAM format)
union PSXColor {
    struct {
        uint16_t r : 5;
        uint16_t g : 5;
        uint16_t b : 5;
        uint16_t k : 1;
    };
    uint16_t raw;

    INLINE uint16_t rev() const { return (r << 11) | (g << 6) | (b << 1) | k; }

    PSXColor() : raw(0) {}

    // 16bit word from VRAM
    PSXColor(uint16_t color) : raw(color) {}

    // 8bit input values
    PSXColor(uint8_t r, uint8_t g, uint8_t b) {
        this->r = r >> 3;
        this->g = g >> 3;
        this->b = b >> 3;
        this->k = 0;
    }

    // 5bit input values
    PSXColor(uint8_t r, uint8_t g, uint8_t b, uint8_t k) : r(r), g(g), b(b), k(k) {}

    PSXColor operator*(const glm::ivec3& rhs) const {
        return PSXColor(                               //
            clamp_top<uint8_t>((rhs.r * r) >> 7, 31),  //
            clamp_top<uint8_t>((rhs.g * g) >> 7, 31),  //
            clamp_top<uint8_t>((rhs.b * b) >> 7, 31),  //
            k                                          //
        );
    }

    INLINE static PSXColor blend(const PSXColor& bg, const PSXColor& c, const gpu::SemiTransparency& transparency) {
        switch (transparency) {
            case gpu::SemiTransparency::Bby2plusFby2:
                return PSXColor(                       //
                    clamp_top((bg.r + c.r) >> 1, 31),  //
                    clamp_top((bg.g + c.g) >> 1, 31),  //
                    clamp_top((bg.b + c.b) >> 1, 31),  //
                    c.k                                //
                );
            case gpu::SemiTransparency::BplusF:
                return PSXColor(                //
                    clamp_top(bg.r + c.r, 31),  //
                    clamp_top(bg.g + c.g, 31),  //
                    clamp_top(bg.b + c.b, 31),  //
                    c.k                         //
                );
            case gpu::SemiTransparency::BminusF:
                return PSXColor(                  //
                    clamp_bottom(bg.r - c.r, 0),  //
                    clamp_bottom(bg.g - c.g, 0),  //
                    clamp_bottom(bg.b - c.b, 0),  //
                    c.k                           //
                );
            case gpu::SemiTransparency::BplusFby4:
                return PSXColor(                       //
                    clamp_top(bg.r + (c.r >> 2), 31),  //
                    clamp_top(bg.g + (c.g >> 2), 31),  //
                    clamp_top(bg.b + (c.b >> 2), 31),  //
                    c.k                                //
                );
        }
    }
};

constexpr uint32_t to15bit(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t newColor = 0;
    newColor |= (r & 0xf8) >> 3;
    newColor |= (g & 0xf8) << 2;
    newColor |= (b & 0xf8) << 7;
    return newColor;
}

inline uint32_t to15bit(uint32_t color) {
    RGB rgb = {};
    rgb.raw = color;

    PSXColor c;
    c.r = rgb.r >> 3;
    c.g = rgb.g >> 3;
    c.b = rgb.b >> 3;
    c.k = 0;
    return c.raw;
}

constexpr uint32_t to24bit(uint16_t color) {
    uint32_t newColor = 0;
    newColor |= (color & 0x7c00) << 1;
    newColor |= (color & 0x3e0) >> 2;
    newColor |= (color & 0x1f) << 19;
    return newColor;
}