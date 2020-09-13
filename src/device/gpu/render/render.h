#pragma once
#include "device/gpu/gpu.h"

class Render {
   public:
    static void drawLine(gpu::GPU* gpu, const primitive::Line& line);
    static void drawTriangle(gpu::GPU* gpu, const primitive::Triangle& triangle);
    static void drawRectangle(gpu::GPU* gpu, const primitive::Rect& rect);
    static void drawDebugDot(gpu::GPU* gpu, PSXColor& color, int32_t x, int32_t y);
    static void drawDebugCross(gpu::GPU* gpu, PSXColor& color, int32_t x, int32_t y);
    static void drawDebugLine(gpu::GPU* gpu, PSXColor& color, primitive::Line& line);
    static void drawDebugCrosses(gpu::GPU* gpu, PSXColor& color, std::vector<ivec2>& positions);
    static void drawDebugLines(gpu::GPU* gpu, PSXColor& color, std::vector<primitive::Line>& lines);
};