#include <algorithm>
#include "device/gpu/psx_color.h"
#include "dither.h"
#include "render.h"
#include "texture_utils.h"
#include "utils/macros.h"
#include "utils/screenshot.h"

#undef VRAM
#define VRAM ((uint16_t(*)[gpu::VRAM_WIDTH])gpu->vram.data())

void Render::drawDebugDot(gpu::GPU* gpu, PSXColor& color, int32_t x, int32_t y) {
    x += gpu->drawingArea.left + gpu->drawingOffsetX;  // gpu->drawingArea.left;
    y += gpu->drawingArea.top + gpu->drawingOffsetY;   // gpu->drawingArea.right;
    if (!gpu->insideDrawingArea(x, y)) {
        return;
    }
    VRAM[y][x] = color.raw;
    // VRAM[y + 256][x] = color.raw;
}

void Render::drawDebugCross(gpu::GPU* gpu, PSXColor& color, int32_t x, int32_t y) {
    drawDebugDot(gpu, color, x, y);
    drawDebugDot(gpu, color, x, y);
    drawDebugDot(gpu, color, x, y - 1);
    drawDebugDot(gpu, color, x, y + 1);
    drawDebugDot(gpu, color, x - 1, y);
    drawDebugDot(gpu, color, x + 1, y);
}

void Render::drawDebugCrosses(gpu::GPU* gpu, PSXColor& color, std::vector<ivec2>& positions) {
    for (auto it = positions.begin(); it != positions.end(); it++) {
        drawDebugCross(gpu, color, it->x, it->y);
    }
}

void Render::drawDebugLine(gpu::GPU* gpu, PSXColor& color, primitive::Line& line) {
    int x0 = line.pos[0].x;
    int y0 = line.pos[0].y;
    int x1 = line.pos[1].x;
    int y1 = line.pos[1].y;

    x0 += gpu->drawingArea.left + gpu->drawingOffsetX;  // gpu->drawingArea.left;
    y0 += gpu->drawingArea.top + gpu->drawingOffsetY;   // gpu->drawingArea.right;
    x1 += gpu->drawingArea.left + gpu->drawingOffsetX;  // gpu->drawingArea.left;
    y1 += gpu->drawingArea.top + gpu->drawingOffsetY;   // gpu->drawingArea.right;
    if (!gpu->insideDrawingArea(x0, y0) || !gpu->insideDrawingArea(x1, y1)) {
        return;
    }

    // TODO: Clip line in drawRectangle
    // Skip rendering when distance between vertices is bigger than 1023x511
    if (abs(x0 - x1) >= 1024) return;
    if (abs(y0 - y1) >= 512) return;

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
    int error = !steep;
    int _y = y0;

    float length = sqrtf(powf(x1 - x0, 2) + powf(y1 - y0, 2));

    auto putPixel = [&](int x, int y) {
        VRAM[y][x] = color.raw;
        // VRAM[y + 256][x] = color.raw;
    };

    for (int _x = x0; _x <= x1; _x++) {
        if (steep) {
            // if (_x > 0 && _x < gpu->gp1_08.getHorizontalResoulution() - 1 && _y > 0 && _y < gpu->gp1_08.getVerticalResoulution() - 1) {
            putPixel(_y, _x);
            //}
        } else {
            // if (_x > 0 && _x < gpu->gp1_08.getHorizontalResoulution() - 1 && _y > 0 && _y < gpu->gp1_08.getVerticalResoulution() - 1) {
            putPixel(_x, _y);
            //}
        }
        error += derror;
        if (error > dx) {
            _y += (y1 > y0 ? 1 : -1);
            error -= dx * 2;
        }
    }
}

void Render::drawDebugLines(gpu::GPU* gpu, PSXColor& color, std::vector<primitive::Line>& lines) {
    for (auto it = lines.begin(); it != lines.end(); it++) {
        Render::drawDebugLine(gpu, color, *it);
    }
}