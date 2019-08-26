#pragma once
#include "device/gpu/gpu.h"

class Render {
   public:
    static void drawLine(gpu::GPU* gpu, const primitive::Line& line);
    static void drawTriangle(gpu::GPU* gpu, const primitive::Triangle& triangle);
    static void drawRectangle(gpu::GPU* gpu, const primitive::Rect& rect);
};