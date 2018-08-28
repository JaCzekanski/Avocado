#include "render.h"
#include <algorithm>

#undef VRAM
#define VRAM ((uint16_t(*)[gpu::VRAM_WIDTH])gpu->vram.data())

void Render::drawLine(gpu::GPU* gpu, const int16_t x[2], const int16_t y[2], const RGB c[2]) {
    int x0 = x[0] + gpu->drawingOffsetX;
    int y0 = y[0] + gpu->drawingOffsetY;
    int x1 = x[1] + gpu->drawingOffsetX;
    int y1 = y[1] + gpu->drawingOffsetY;

    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int dx = x1 - x0;
    int dy = y1 - y0;
    int derror = std::abs(dy) * 2;
    int error = 0;
    int _y = y0;
    for (int _x = x0; _x <= x1; _x++) {
        if (steep) {
            if (gpu->insideDrawingArea(_y, _x)) VRAM[_x][_y] = to15bit(c[0].raw);
        } else {
            if (gpu->insideDrawingArea(_x, _y)) VRAM[_y][_x] = to15bit(c[0].raw);
        }
        error += derror;
        if (error > dx) {
            _y += (y1 > y0 ? 1 : -1);
            error -= dx * 2;
        }
    }
}
