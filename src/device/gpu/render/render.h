#pragma once
#include "device/gpu/gpu.h"

class Render {
   public:
    static void drawLine(gpu::GPU* gpu, const primitive::Line& line);
    static void drawTriangle(gpu::GPU* gpu, gpu::Vertex v[3]);
    static void drawRectangle(gpu::GPU* gpu, const primitive::Rect& rect);
};