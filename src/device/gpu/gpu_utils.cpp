#include <algorithm>
#include "gpu.h"

int GPU::minDrawingX(int x) const { return std::max((int)drawingArea.left, std::max(0, x)); }

int GPU::minDrawingY(int y) const { return std::max((int)drawingArea.top, std::max(0, y)); }

int GPU::maxDrawingX(int x) const { return std::min((int)drawingArea.right, std::min(VRAM_WIDTH, x)); }

int GPU::maxDrawingY(int y) const { return std::min((int)drawingArea.bottom, std::min(VRAM_HEIGHT, y)); }

bool GPU::insideDrawingArea(int x, int y) const {
    return (x >= drawingArea.left) && (x < drawingArea.right) && (x < VRAM_WIDTH) && (y >= drawingArea.top) && (y < drawingArea.bottom)
           && (y < VRAM_HEIGHT);
}