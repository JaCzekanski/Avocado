#pragma once
#include <cstdint>
#include "math_utils.h"

inline uint32_t to15bit(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t newColor = 0;
    newColor |= (r & 0xf8) >> 3;
    newColor |= (g & 0xf8) << 2;
    newColor |= (b & 0xf8) << 7;
    return newColor;
}

inline uint32_t to15bit(uint32_t color) {
    uint32_t newColor = 0;
    newColor |= (color & 0xf80000) >> 19;
    newColor |= (color & 0xf800) >> 6;
    newColor |= (color & 0xf8) << 7;
    return newColor;
}

inline uint32_t to24bit(uint16_t color) {
    uint32_t newColor = 0;
    newColor |= (color & 0x7c00) << 1;
    newColor |= (color & 0x3e0) >> 2;
    newColor |= (color & 0x1f) << 19;
    return newColor;
}

union RGB {
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t _;
    };
    uint32_t c;
};

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