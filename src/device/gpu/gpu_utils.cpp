#pragma once
#include <algorithm>
#include "gpu.h"

int GPU::minDrawingX(int x) const { return std::max((int)drawingAreaLeft, std::max(0, x)); }

int GPU::minDrawingY(int y) const { return std::max((int)drawingAreaTop, std::max(0, y)); }

int GPU::maxDrawingX(int x) const { return std::min((int)drawingAreaRight, std::min(GPU::vramWidth, x)); }

int GPU::maxDrawingY(int y) const { return std::min((int)drawingAreaBottom, std::min(GPU::vramHeight, y)); }

bool GPU::insideDrawingArea(int x, int y) const {
    return (x >= drawingAreaLeft) && (x < drawingAreaRight) && (x < vramWidth) && (y >= drawingAreaTop) && (y < drawingAreaBottom)
           && (y < vramHeight);
}

uint32_t GPU::to15bit(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t newColor = 0;
    newColor |= (r & 0xf8) >> 3;
    newColor |= (g & 0xf8) << 2;
    newColor |= (b & 0xf8) << 7;
    return newColor;
}

uint32_t GPU::to15bit(uint32_t color) {
    uint32_t newColor = 0;
    newColor |= (color & 0xf80000) >> 19;
    newColor |= (color & 0xf800) >> 6;
    newColor |= (color & 0xf8) << 7;
    return newColor;
}

uint32_t GPU::to24bit(uint16_t color) {
    uint32_t newColor = 0;
    newColor |= (color & 0x7c00) << 1;
    newColor |= (color & 0x3e0) >> 2;
    newColor |= (color & 0x1f) << 19;
    return newColor;
}