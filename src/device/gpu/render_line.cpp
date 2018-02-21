#include <algorithm>
#include "gpu.h"
#include "math_utils.h"

void GPU::drawLine(int16_t x[2], int16_t y[2], uint32_t c[2]) {
    x[0] += drawingOffsetX;
    y[0] += drawingOffsetY;
    x[1] += drawingOffsetX;
    y[1] += drawingOffsetY;

    bool steep = false;
    if (std::abs(x[0] - x[1]) < std::abs(y[0] - y[1])) {
        std::swap(x[0], y[0]);
        std::swap(x[1], y[1]);
        steep = true;
    }
    if (x[0] > x[1]) {
        std::swap(x[0], x[1]);
        std::swap(y[0], y[1]);
    }

    int dx = x[1] - x[0];
    int dy = y[1] - y[0];
    int derror = std::abs(dy) * 2;
    int error = 0;
    int _y = y[0];
    for (int _x = x[0]; _x <= x[1]; _x++) {
        if (steep) {
            if (insideDrawingArea(_y, _x)) VRAM[_x][_y] = to15bit(c[0]);
        } else {
            if (insideDrawingArea(_x, _y)) VRAM[_y][_x] = to15bit(c[0]);
        }
        error += derror;
        if (error > dx) {
            _y += (y[1] > y[0] ? 1 : -1);
            error -= dx * 2;
        }
    }
}
