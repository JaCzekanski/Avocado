#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include "utils/math.h"

// Union for storing 24bit color (used in GPU commands)
union RGB {
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t _;
    };
    uint32_t raw;
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

    PSXColor() : raw(0) {}
    PSXColor(uint16_t color) : raw(color) {}
    PSXColor(uint8_t r, uint8_t g, uint8_t b) {
        this->r = r >> 3;
        this->g = g >> 3;
        this->b = b >> 3;
        this->k = 0;
    }

    PSXColor operator+(const PSXColor& rhs) {
        r = std::min(r + rhs.r, 31);
        g = std::min(g + rhs.g, 31);
        b = std::min(b + rhs.b, 31);
        return *this;
    }

    PSXColor operator-(const PSXColor& rhs) {
        r = std::max<int>(r - rhs.r, 0);
        g = std::max<int>(g - rhs.g, 0);
        b = std::max<int>(b - rhs.b, 0);
        return *this;
    }

    PSXColor operator*(const float& rhs) {
        r = (uint16_t)(r * rhs);
        g = (uint16_t)(g * rhs);
        b = (uint16_t)(b * rhs);
        return *this;
    }

    PSXColor operator/(const int& rhs) {
        r = (uint16_t)(r / rhs);
        g = (uint16_t)(g / rhs);
        b = (uint16_t)(b / rhs);
        return *this;
    }

    PSXColor operator>>(const int& sh) {
        r >>= sh;
        g >>= sh;
        b >>= sh;
        return *this;
    }

    PSXColor operator*(const glm::vec3& rhs) {
        r = std::min<uint16_t>((uint16_t)(rhs.r * r), 31);
        g = std::min<uint16_t>((uint16_t)(rhs.g * g), 31);
        b = std::min<uint16_t>((uint16_t)(rhs.b * b), 31);
        return *this;
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