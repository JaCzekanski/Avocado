#pragma once
#include <algorithm>
#include "gpu.h"

int GPU::minDrawingX(int x) const { return std::max((int)drawingAreaLeft, std::max(0, x)); }

int GPU::minDrawingY(int y) const { return std::max((int)drawingAreaTop, std::max(0, y)); }

int GPU::maxDrawingX(int x) const { return std::min((int)drawingAreaRight, std::min(GPU::VRAM_WIDTH, x)); }

int GPU::maxDrawingY(int y) const { return std::min((int)drawingAreaBottom, std::min(GPU::VRAM_HEIGHT, y)); }

bool GPU::insideDrawingArea(int x, int y) const {
    return (x >= drawingAreaLeft) && (x < drawingAreaRight) && (x < VRAM_WIDTH) && (y >= drawingAreaTop) && (y < drawingAreaBottom)
           && (y < VRAM_HEIGHT);
}

uint16_t GPU::tex4bit(glm::ivec2 tex, glm::ivec2 texPage, glm::ivec2 clut) {
    uint16_t index = VRAM[texPage.y + tex.y][texPage.x + tex.x / 4];
    uint16_t entry = (index >> ((tex.x & 3) * 4)) & 0xf;
    return VRAM[clut.y][clut.x + entry];
}

uint16_t GPU::tex8bit(glm::ivec2 tex, glm::ivec2 texPage, glm::ivec2 clut) {
    uint16_t index = VRAM[texPage.y + tex.y][texPage.x + tex.x / 2];
    uint16_t entry = (index >> ((tex.x & 1) * 8)) & 0xff;
    return VRAM[clut.y][clut.x + entry];
}