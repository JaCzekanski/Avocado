#include <algorithm>
#include "dither.h"
#include "render.h"
#include "utils/macros.h"

#undef VRAM
#define VRAM ((uint16_t(*)[gpu::VRAM_WIDTH])gpu->vram.data())

void Render::drawLine(gpu::GPU* gpu, const primitive::Line& line) {
    using Transparency = gpu::GP0_E1::SemiTransparency;
    const auto transparency = gpu->gp0_e1.semiTransparency;
    const bool checkMaskBeforeDraw = gpu->gp0_e6.checkMaskBeforeDraw;
    const bool setMaskWhileDrawing = gpu->gp0_e6.setMaskWhileDrawing;
    const bool dithering = gpu->gp0_e1.dither24to15;

    int x0 = line.pos[0].x + gpu->drawingOffsetX;
    int y0 = line.pos[0].y + gpu->drawingOffsetY;
    int x1 = line.pos[1].x + gpu->drawingOffsetX;
    int y1 = line.pos[1].y + gpu->drawingOffsetY;
    RGB c0 = line.color[0];
    RGB c1 = line.color[1];

    // TODO: Clip line in drawRectangle

    // Skip rendering when distence between vertices is bigger than 1023x511
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
        std::swap(c0, c1);
    }

    int dx = x1 - x0;
    int dy = y1 - y0;
    int derror = std::abs(dy) * 2;
    int error = 0;
    int _y = y0;

    float length = sqrtf(powf(x1 - x0, 2) + powf(y1 - y0, 2));

    // TODO: Precalculate color stepping
    auto getColor = [&](int x, int y) -> RGB {
        if (!line.gouroudShading) {
            return c0;
        }
        float relPos = sqrtf(powf(x0 - x, 2) + powf(y0 - y, 2));

        float progress = relPos / length;

        return c0 * (1.f - progress) + c1 * progress;
    };

    auto putPixel = [&](int x, int y, RGB fullColor) {
        if (unlikely(checkMaskBeforeDraw)) {
            PSXColor bg = VRAM[y][x];
            if (bg.k) return;
        }

        PSXColor c(fullColor.r, fullColor.g, fullColor.b);
        if (dithering) {
            glm::ivec3 col(fullColor.r, fullColor.g, fullColor.b);
            col += ditherTable[y & 3u][x & 3u];
            col = glm::clamp(col, 0, 255);

            c = PSXColor(col.r, col.g, col.b);
        }

        // TODO: This code repeats 3 times, make it more generic
        if (line.isSemiTransparent) {
            PSXColor bg = VRAM[y][x];
            switch (transparency) {
                case Transparency::Bby2plusFby2: c = (bg >> 1) + (c >> 1); break;
                case Transparency::BplusF: c = bg + c; break;
                case Transparency::BminusF: c = bg - c; break;
                case Transparency::BplusFby4: c = bg + (c >> 2); break;
            }
        }

        c.k |= setMaskWhileDrawing;

        VRAM[y][x] = c.raw;
    };

    for (int _x = x0; _x <= x1; _x++) {
        if (steep) {
            // TODO: Remove insideDrawingArea calls
            if (gpu->insideDrawingArea(_y, _x)) putPixel(_y, _x, getColor(_x, _y));
        } else {
            if (gpu->insideDrawingArea(_x, _y)) putPixel(_x, _y, getColor(_x, _y));
        }
        error += derror;
        if (error > dx) {
            _y += (y1 > y0 ? 1 : -1);
            error -= dx * 2;
        }
    }
}
