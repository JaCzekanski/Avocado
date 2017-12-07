#include "gpu.h"
#include <cstdio>
#include <cassert>
#include <algorithm>
#include <glm/glm.hpp>
#include <imgui.h>

#define VRAM ((uint16_t(*)[vramWidth])vram.data())

namespace device {
namespace gpu {

const char* CommandStr[] = {"None",           "FillRectangle",  "Polygon",       "Line",           "Rectangle",
                            "CopyCpuToVram1", "CopyCpuToVram2", "CopyVramToCpu", "CopyVramToVram", "Extra"};

template <typename T>
T clamp(T number, size_t range) {
    if (number > range) number = range;
    return number;
}

void GPU::reset() {
    irqRequest = false;
    displayDisable = true;
    dmaDirection = 0;
    displayAreaStartX = 0;
    displayAreaStartY = 0;
    displayRangeX1 = 0x200;
    displayRangeX2 = 0x200 + 256 * 10;
    displayRangeY1 = 0x10;
    displayRangeY2 = 0x10 + 240;

    gp1_08._reg = 0;
    gp0_e1._reg = 0;
    gp0_e2._reg = 0;

    drawingAreaLeft = 0;
    drawingAreaTop = 0;
    drawingAreaRight = 0;
    drawingAreaBottom = 0;
    drawingOffsetX = 0;
    drawingOffsetY = 0;

    gp0_e6._reg = 0;
}

void GPU::drawPolygon(int x[4], int y[4], RGB c[4], TextureInfo t, bool isFourVertex, bool textured, int flags) {
    int baseX = 0;
    int baseY = 0;

    int clutX = 0;
    int clutY = 0;

    int bitcount = 0;

    if (textured) {
        // t[0] ClutYyXx
        // t[1] PageYyXx
        // t[2] 0000YyXx
        // t[3] 0000YyXx

        // TODO: struct
        clutX = ((t.palette & 0x003f0000) >> 16) * 16;
        clutY = ((t.palette & 0x7fc00000) >> 22);

        baseX = ((t.texpage & 0x0f0000) >> 16) * 64;   // N * 64
        baseY = ((t.texpage & 0x100000) >> 20) * 256;  // N * 256

        int transparency = (t.texpage & 0x600000) >> 21;

        flags |= (transparency) << 6;

        int depth = (t.texpage & 0x1800000) >> 23;
        if (depth == 0) bitcount = 4;
        if (depth == 1) bitcount = 8;
        if (depth == 2 || depth == 3) bitcount = 16;
    }

    Vertex v[3];
    for (int i : {0, 1, 2}) {
        v[i] = {{x[i], y[i]}, {c[i].r, c[i].g, c[i].b}, {t.uv[i].x, t.uv[i].y}, bitcount, {clutX, clutY}, {baseX, baseY}, flags};
        //        renderList.push_back(v[i]);
    }
    drawTriangle(v);

    if (isFourVertex) {
        for (int i : {1, 2, 3}) {
            v[i - 1] = {{x[i], y[i]}, {c[i].r, c[i].g, c[i].b}, {t.uv[i].x, t.uv[i].y}, bitcount, {clutX, clutY}, {baseX, baseY}, flags};
            //            renderList.push_back(v[i - 1]);
        }
        drawTriangle(v);
    }

#undef texX
#undef texY
}

void GPU::cmdFillRectangle(const uint8_t command, uint32_t arguments[]) {
    startX = currX = arguments[1] & 0xffff;
    startY = currY = (arguments[1] & 0xffff0000) >> 16;
    endX = startX + (arguments[2] & 0xffff);
    endY = startY + ((arguments[2] & 0xffff0000) >> 16);

    uint32_t color = to15bit(arguments[0] & 0xffffff);

    for (;;) {
        VRAM[currY % 512][currX % 1024] = color;

        if (currX++ >= endX) {
            currX = startX;
            if (++currY >= endY) break;
        }
    }

    cmd = Command::None;
}

void GPU::cmdPolygon(const PolygonArgs arg, uint32_t arguments[]) {
    int ptr = 1;
    int x[4], y[4] = {0};
    RGB c[4] = {0};
    TextureInfo tex;
    for (int i = 0; i < arg.getVertexCount(); i++) {
        x[i] = (int32_t)(int16_t)(arguments[ptr] & 0xffff);
        y[i] = (int32_t)(int16_t)((arguments[ptr++] & 0xffff0000) >> 16);

        if (!arg.isRawTexture && (!arg.gouroudShading || i == 0)) c[i].c = arguments[0] & 0xffffff;
        if (arg.isTextureMapped) {
            if (i == 0) tex.palette = arguments[ptr];
            if (i == 1) tex.texpage = arguments[ptr];
            tex.uv[i].x = arguments[ptr] & 0xff;
            tex.uv[i].y = (arguments[ptr] >> 8) & 0xff;
            ptr++;
        }
        if (arg.gouroudShading && i < arg.getVertexCount() - 1) c[i + 1].c = arguments[ptr++];
    }
    int flags = 0;
    if (arg.semiTransparency) flags |= Vertex::SemiTransparency;
    if (arg.isRawTexture) flags |= Vertex::RawTexture;
    drawPolygon(x, y, c, tex, arg.isQuad, arg.isTextureMapped, flags);

    cmd = Command::None;
}

template <typename T>
T clamp(T v, T min, T max) {
    return std::min(std::max(v, min), max);
}

void GPU::drawLine(int x0, int y0, int x1, int y1, int c0, int c1) {
    x0 += drawingOffsetX;
    y0 += drawingOffsetY;
    x1 += drawingOffsetX;
    y1 += drawingOffsetY;
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
    int derror2 = std::abs(dy) * 2;
    int error2 = 0;
    int y = y0;
    for (int x = x0; x <= x1; x++) {
        if (steep) {
            VRAM[clamp(x, 0, 511)][clamp(y, 0, 1023)] = to15bit(c0);
        } else {
            VRAM[clamp(y, 0, 511)][clamp(x, 0, 1023)] = to15bit(c0);
        }
        error2 += derror2;
        if (error2 > dx) {
            y += (y1 > y0 ? 1 : -1);
            error2 -= dx * 2;
        }
    }
}

void GPU::cmdLine(const LineArgs arg, uint32_t arguments[]) {
    int ptr = 1;
    int sx = 0, sy = 0, sc = 0;
    int ex = 0, ey = 0, ec = 0;
    for (int i = 0; i < arg.getArgumentCount() - 1; i++) {
        if (arguments[ptr] == 0x55555555) break;
        if (i == 0) {
            sx = (int32_t)(int16_t)(arguments[ptr] & 0xffff);
            sy = (int32_t)(int16_t)((arguments[ptr++] & 0xffff0000) >> 16);
            sc = arguments[0] & 0xffffff;
        } else {
            sx = ex;
            sy = ey;
            sc = ec;
        }

        if (arg.gouroudShading)
            ec = arguments[ptr++];
        else
            ec = arguments[0] & 0xffffff;
        ex = (int32_t)(int16_t)(arguments[ptr] & 0xffff);
        ey = (int32_t)(int16_t)((arguments[ptr++] & 0xffff0000) >> 16);

        int x[4] = {sx, sx + 1, ex + 1, ex};
        int y[4] = {sy, sy + 1, ey + 1, ey};
        RGB c[4];
        c[0].c = sc;
        c[1].c = sc;
        c[2].c = ec;
        c[3].c = ec;

        // TODO: Switch to proprer line rendering
        // TODO:           ^^^^^^^ fix typo

        //        int flags = 0;
        //        if (arg.semiTransparency) flags |= Vertex::SemiTransparency;
        //                drawPolygon(x, y, c, nullptr, false, false, flags);

        // No transparency support
        // No Gouroud Shading

        drawLine(sx, sy, ex, ey, sc, ec);
    }

    cmd = Command::None;
}

void GPU::cmdRectangle(const RectangleArgs arg, uint32_t arguments[]) {
    int w = arg.getSize();
    int h = arg.getSize();

    if (arg.size == 0) {
        w = (int32_t)(int16_t)(arguments[(arg.isTextureMapped ? 3 : 2)] & 0xffff);
        h = (int32_t)(int16_t)((arguments[(arg.isTextureMapped ? 3 : 2)] & 0xffff0000) >> 16);
    }

    int x = (int32_t)(int16_t)(arguments[1] & 0xffff);
    int y = (int32_t)(int16_t)((arguments[1] & 0xffff0000) >> 16);

    int _x[4] = {x, x + w, x, x + w};
    int _y[4] = {y, y, y + h, y + h};
    RGB _c[4];
    _c[0].c = arguments[0];
    _c[1].c = arguments[0];
    _c[2].c = arguments[0];
    _c[3].c = arguments[0];
    TextureInfo tex;

    if (arg.isTextureMapped) {
        int texX = arguments[2] & 0xff;
        int texY = (arguments[2] & 0xff00) >> 8;

        tex.palette = arguments[2];
        tex.texpage = (gp0_e1._reg << 16);

        tex.uv[0].x = texX;
        tex.uv[0].y = texY;

        tex.uv[1].x = texX + w;
        tex.uv[1].y = texY;

        tex.uv[2].x = texX;
        tex.uv[2].y = texY + h;

        tex.uv[3].x = texX + w;
        tex.uv[3].y = texY + h;
    }
    int flags = 0;
    if (arg.semiTransparency) flags |= Vertex::SemiTransparency;
    if (arg.isRawTexture) flags |= Vertex::RawTexture;
    drawPolygon(_x, _y, _c, tex, true, arg.isTextureMapped, flags);

    cmd = Command::None;
}

void GPU::cmdCpuToVram1(const uint8_t command, uint32_t arguments[]) {
    if ((arguments[0] & 0x00ffffff) != 0) {
        printf("cmdCpuToVram1: Suspicious arg0: 0x%x\n", arguments[0]);
    }
    startX = currX = arguments[1] & 0xffff;
    startY = currY = (arguments[1] & 0xffff0000) >> 16;

    endX = startX + (arguments[2] & 0xffff);
    endY = startY + ((arguments[2] & 0xffff0000) >> 16);

    cmd = Command::CopyCpuToVram2;
    argumentCount = 1;
    currentArgument = 0;
}

void GPU::cmdCpuToVram2(const uint8_t command, uint32_t arguments[]) {
    uint32_t byte = arguments[0];

    // TODO: ugly code
    VRAM[currY % 512][currX++ % 1024] = byte & 0xffff;
    if (currX >= endX) {
        currX = startX;
        if (++currY >= endY) cmd = Command::None;
    }

    VRAM[currY % 512][currX++ % 1024] = (byte >> 16) & 0xffff;
    if (currX >= endX) {
        currX = startX;
        if (++currY >= endY) cmd = Command::None;
    }

    currentArgument = 0;
}

void GPU::cmdVramToCpu(const uint8_t command, uint32_t arguments[]) {
    if ((arguments[0] & 0x00ffffff) != 0) {
        printf("cmdVramToCpu: Suspicious arg0: 0x%x\n", arguments[0]);
    }
    gpuReadMode = 1;
    startX = currX = arguments[1] & 0xffff;
    startY = currY = (arguments[1] & 0xffff0000) >> 16;
    endX = startX + (arguments[2] & 0xffff);
    endY = startY + ((arguments[2] & 0xffff0000) >> 16);

    cmd = Command::None;
}

void GPU::cmdVramToVram(const uint8_t command, uint32_t arguments[]) {
    if ((arguments[0] & 0x00ffffff) != 0) {
        printf("cpuVramToVram: Suspicious arg0: 0x%x\n", arguments[0]);
    }
    int srcX = arguments[1] & 0xffff;
    int srcY = (arguments[1] & 0xffff0000) >> 16;

    int dstX = arguments[2] & 0xffff;
    int dstY = (arguments[2] & 0xffff0000) >> 16;

    int width = (arguments[3] & 0xffff);
    int height = ((arguments[3] & 0xffff0000) >> 16);

    if (width > 1024 || height > 512) {
        printf("cpuVramToVram: Suspicious width: 0x%x or height: 0x%x\n", width, height);
        cmd = Command::None;
        return;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // TODO: boundary check!
            VRAM[(dstY + y) % 512][(dstX + x) % 1024] = VRAM[(srcY + y) % 512][(srcX + x) % 1024];
        }
    }

    cmd = Command::None;
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

void GPU::step() {
    uint8_t dataRequest = 0;
    if (dmaDirection == 0)
        dataRequest = 0;
    else if (dmaDirection == 1)
        dataRequest = 1;  // FIFO not full
    else if (dmaDirection == 2)
        dataRequest = 1;  // Same as bit28, ready to receive dma block
    else if (dmaDirection == 3)
        dataRequest = cmd != Command::CopyCpuToVram2;  // Same as bit27, ready to send VRAM to CPU

    GPUSTAT = (gp0_e1._reg & 0x7FF) | (gp0_e6.setMaskWhileDrawing << 11) | (gp0_e6.checkMaskBeforeDraw << 12) | (1 << 13)  // always set
              | ((uint8_t)gp1_08.reverseFlag << 14) | ((uint8_t)gp0_e1.textureDisable << 15) | ((uint8_t)gp1_08.horizontalResolution2 << 16)
              | ((uint8_t)gp1_08.horizontalResolution1 << 17) | ((uint8_t)gp1_08.verticalResolution << 19)
              | ((uint8_t)gp1_08.videoMode << 20) | ((uint8_t)gp1_08.colorDepth << 21) | (gp1_08.interlace << 22) | (displayDisable << 23)
              | (irqRequest << 24) | (dataRequest << 25) | (1 << 26)  // Ready for DMA command
              | ((cmd != Command::CopyCpuToVram2) << 27) | (1 << 28)  // Ready for receive DMA block
              | ((dmaDirection & 3) << 29) | (odd << 31);
}

uint32_t GPU::read(uint32_t address) {
    int reg = address & 0xfffffffc;
    if (reg == 0) {
        if (gpuReadMode == 0 || gpuReadMode == 2) return GPUREAD;
        if (gpuReadMode == 1) {
            uint32_t word = VRAM[currY][currX] | (VRAM[currY][currX + 1] << 16);
            currX += 2;

            if (currX >= endX) {
                currX = startX;
                if (++currY >= endY) {
                    gpuReadMode = 0;
                }
            }
            return word;
        }
    }
    if (reg == 4) {
        step();
        return GPUSTAT;
    }
    return 0;
}

void GPU::write(uint32_t address, uint32_t data) {
    int reg = address & 0xfffffffc;
    if (reg == 0) writeGP0(data);
    if (reg == 4) writeGP1(data);
}

void GPU::writeGP0(uint32_t data) {
    if (cmd == Command::None) {
        command = data >> 24;
        arguments[0] = data & 0xffffff;
        argumentCount = 0;
        currentArgument = 1;

        if (command == 0x00) {
            // NOP
        } else if (command == 0x01) {
            // Clear Cache
        } else if (command == 0x02) {
            // Fill rectangle
            cmd = Command::FillRectangle;
            argumentCount = 2;
        } else if (command >= 0x20 && command < 0x40) {
            // Polygons
            cmd = Command::Polygon;
            argumentCount = PolygonArgs(command).getArgumentCount();
        } else if (command >= 0x40 && command < 0x60) {
            // Lines
            cmd = Command::Line;
            argumentCount = LineArgs(command).getArgumentCount();
        } else if (command >= 0x60 && command < 0x80) {
            // Rectangles
            cmd = Command::Rectangle;
            argumentCount = RectangleArgs(command).getArgumentCount();
        } else if (command == 0xa0) {
            // Copy rectangle (CPU -> VRAM)
            cmd = Command::CopyCpuToVram1;
            argumentCount = 2;
        } else if (command == 0xc0) {
            // Copy rectangle (VRAM -> CPU)
            cmd = Command::CopyVramToCpu;
            argumentCount = 2;
        } else if (command == 0x80) {
            // Copy rectangle (VRAM -> VRAM)
            cmd = Command::CopyVramToVram;
            argumentCount = 3;
        } else if (command == 0xe1) {
            // Draw mode setting
            gp0_e1._reg = arguments[0];
        } else if (command == 0xe2) {
            // Texture window setting
            gp0_e2._reg = arguments[0];
        } else if (command == 0xe3) {
            // Drawing area top left
            drawingAreaLeft = arguments[0] & 0x3ff;
            drawingAreaTop = (arguments[0] & 0xffc00) >> 10;
        } else if (command == 0xe4) {
            // Drawing area bottom right
            drawingAreaRight = arguments[0] & 0x3ff;
            drawingAreaBottom = (arguments[0] & 0xffc00) >> 10;
        } else if (command == 0xe5) {
            // Drawing offset
            drawingOffsetX = ((int16_t)((arguments[0] & 0x7ff) << 5)) >> 5;
            drawingOffsetY = ((int16_t)(((arguments[0] & 0x3FF800) >> 11) << 5)) >> 5;
        } else if (command == 0xe6) {
            // Mask bit setting
            gp0_e6._reg = arguments[0];
        } else if (command == 0x1f) {
            // Interrupt request
            irqRequest = true;
            // TODO: IRQ
        } else {
            printf("GP0(0x%02x) args 0x%06x\n", command, arguments[0]);
        }

        if (gpuLogEnabled && cmd == Command::None) {
            GPU_LOG_ENTRY entry;
            entry.cmd = Command::Extra;
            entry.command = command;
            entry.args = std::vector<uint32_t>();
            entry.args.push_back(arguments[0]);
            gpuLogList.push_back(entry);
        }
        // if (cmd == Command::None) printf("GPU: 0x%02x\n", command);

        argumentCount++;
        return;
    }

    if (currentArgument < argumentCount) {
        arguments[currentArgument++] = data;
        if (argumentCount == MAX_ARGS && data == 0x55555555) argumentCount = currentArgument;
        if (currentArgument != argumentCount) return;
    }

    if (gpuLogEnabled && cmd != Command::CopyCpuToVram2) {
        GPU_LOG_ENTRY entry;
        entry.cmd = cmd;
        entry.command = command;
        entry.args = std::vector<uint32_t>(arguments, arguments + argumentCount);
        gpuLogList.push_back(entry);
    }

    // printf("%s(0x%x)\n", CommandStr[(int)cmd], command);

    if (cmd == Command::FillRectangle)
        cmdFillRectangle(command, arguments);
    else if (cmd == Command::Polygon)
        cmdPolygon(command, arguments);
    else if (cmd == Command::Line)
        cmdLine(command, arguments);
    else if (cmd == Command::Rectangle)
        cmdRectangle(command, arguments);
    else if (cmd == Command::CopyCpuToVram1)
        cmdCpuToVram1(command, arguments);
    else if (cmd == Command::CopyCpuToVram2)
        cmdCpuToVram2(command, arguments);
    else if (cmd == Command::CopyVramToCpu)
        cmdVramToCpu(command, arguments);
    else if (cmd == Command::CopyVramToVram)
        cmdVramToVram(command, arguments);
}

void GPU::writeGP1(uint32_t data) {
    uint32_t command = (data >> 24) & 0x3f;
    uint32_t argument = data & 0xffffff;

    if (command == 0x00) {  // Reset GPU
        reset();
    } else if (command == 0x01) {  // Reset command buffer

    } else if (command == 0x02) {  // Acknowledge IRQ1
        irqRequest = false;
    } else if (command == 0x03) {  // Display Enable
        displayDisable = (Bit)(argument & 1);
    } else if (command == 0x04) {  // DMA Direction
        dmaDirection = argument & 3;
    } else if (command == 0x05) {  // Start of display area
        displayAreaStartX = argument & 0x3ff;
        displayAreaStartY = argument >> 10;
    } else if (command == 0x06) {  // Horizontal display range
        displayRangeX1 = argument & 0xfff;
        displayRangeX2 = argument >> 12;
    } else if (command == 0x07) {  // Vertical display range
        displayRangeY1 = argument & 0x3ff;
        displayRangeX2 = argument >> 10;
    } else if (command == 0x08) {  // Display mode
        gp1_08._reg = argument;
    } else if (command == 0x09) {  // Allow texture disable
        textureDisableAllowed = argument & 1;
    } else if (command >= 0x10 && command <= 0x1f) {  // get GPU Info
        gpuReadMode = 2;
        argument &= 0xf;

        if (argument == 2) {
            GPUREAD = gp0_e2._reg;
        } else if (argument == 3) {
            GPUREAD = (drawingAreaTop << 10) | drawingAreaLeft;
        } else if (argument == 4) {
            GPUREAD = (drawingAreaBottom << 10) | drawingAreaRight;
        } else if (argument == 5) {
            GPUREAD = (drawingOffsetY << 11) | drawingOffsetX;
        } else if (argument == 7) {
            GPUREAD = 2;  // GPU Version
        } else if (argument == 8) {
            GPUREAD = 0;
        } else {
            // GPUREAD unchanged
        }
    } else {
        printf("GP1(0x%02x) args 0x%06x\n", command, argument);
        assert(false);
    }
    // command 0x20 is not implemented
}
std::vector<Vertex>& GPU::render() { return renderList; }

bool GPU::emulateGpuCycles(int cycles) {
    const int LINE_VBLANK_START_NTSC = 243;
    const int LINES_TOTAL_NTSC = 263;
    static int gpuLine = 0;
    static int gpuDot = 0;

    gpuDot += cycles;

    int newLines = gpuDot / 3413;
    if (newLines == 0) return false;
    gpuDot %= 3413;
    gpuLine += newLines;

    if (gpuLine < LINE_VBLANK_START_NTSC - 1) {
        if (gp1_08.verticalResolution == GP1_08::VerticalResolution::r480 && gp1_08.interlace) {
            odd = (frames % 2) != 0;
        } else {
            odd = (gpuLine % 2) != 0;
        }
    } else {
        odd = false;
    }

    if (gpuLine == LINES_TOTAL_NTSC - 1) {
        gpuLine = 0;
        frames++;
        return true;
    }
    return false;
}

glm::vec3 barycentric(glm::ivec2 pos[3], glm::ivec2 p) {
    glm::vec3 u = glm::cross(glm::vec3(pos[2].x - pos[0].x, pos[1].x - pos[0].x, pos[0].x - p.x),
                             glm::vec3(pos[2].y - pos[0].y, pos[1].y - pos[0].y, pos[0].y - p.y));
    if (std::abs(u.z) < 1) return glm::vec3(-1.f, 1.f, 1.f);
    return glm::vec3(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
}

inline uint16_t GPU::tex4bit(glm::ivec2 tex, glm::ivec2 texPage, glm::ivec2 clut) {
    uint16_t index = VRAM[texPage.y + tex.y][texPage.x + tex.x / 4];
    uint16_t entry = (index >> ((tex.x & 3) * 4)) & 0xf;
    return VRAM[clut.y][clut.x + entry];
}

inline uint16_t GPU::tex8bit(glm::ivec2 tex, glm::ivec2 texPage, glm::ivec2 clut) {
    uint16_t index = VRAM[texPage.y + tex.y][texPage.x + tex.x / 2];
    uint16_t entry = (index >> ((tex.x & 1) * 8)) & 0xff;
    return VRAM[clut.y][clut.x + entry];
}

float lerp(float a, float b, float t) { return a + t * (b - a); }

union PSXColor {
    struct {
        uint16_t r : 5;
        uint16_t g : 5;
        uint16_t b : 5;
        uint16_t k : 1;
    };
    uint16_t _;

    PSXColor() : _(0) {}
    PSXColor(uint16_t color) : _(color) {}

    PSXColor operator+(PSXColor rhs) {
        r = clamp<int>(r + rhs.r, 0, 31);
        g = clamp<int>(g + rhs.g, 0, 31);
        b = clamp<int>(b + rhs.b, 0, 31);
        return *this;
    }

    PSXColor operator-(PSXColor rhs) {
        r = clamp<int>(r - rhs.r, 0, 31);
        g = clamp<int>(g - rhs.g, 0, 31);
        b = clamp<int>(b - rhs.b, 0, 31);
        return *this;
    }

    PSXColor operator*(float rhs) {
        r *= rhs;
        g *= rhs;
        b *= rhs;
        return *this;
    }

    PSXColor operator/(float rhs) {
        r /= rhs;
        g /= rhs;
        b /= rhs;
        return *this;
    }

    PSXColor operator*(glm::vec3 rhs) {
        r = clamp<int>((rhs.r / 0.5f) * r, 0, 31);
        g = clamp<int>((rhs.g / 0.5f) * g, 0, 31);
        b = clamp<int>((rhs.b / 0.5f) * b, 0, 31);
        return *this;
    }
};

void GPU::triangle(glm::ivec2 pos[3], glm::vec3 color[3], glm::ivec2 tex[3], glm::ivec2 texPage, glm::ivec2 clut, int bits, int flags) {
    for (int i = 0; i < 3; i++) {
        pos[i].x += drawingOffsetX;
        pos[i].y += drawingOffsetY;
    }
    // clang-format off
    glm::ivec2 min = glm::ivec2(
        std::max((int)drawingAreaLeft, std::max(0, std::min({ pos[0].x, pos[1].x, pos[2].x }))),
        std::max((int)drawingAreaTop, std::max(0, std::min({ pos[0].y, pos[1].y, pos[2].y })))
	);
    glm::ivec2 max = glm::ivec2(
        std::min((int)drawingAreaRight, std::min(vramWidth, std::max({ pos[0].x, pos[1].x, pos[2].x }))),
        std::min((int)drawingAreaBottom, std::min(vramHeight, std::max({ pos[0].y, pos[1].y, pos[2].y })))
	);
    // clang-format on

    glm::ivec2 p;

    for (p.y = min.y; p.y < max.y; p.y++) {
        for (p.x = min.x; p.x < max.x; p.x++) {
            glm::vec3 s = barycentric(pos, p);
            if (s.x < 0 || s.y < 0 || s.z < 0) continue;

            PSXColor c;
            if (bits == 0) {
                // clang-format off
                glm::vec3 calculatedColor = glm::vec3(
                    s.x * color[0].r + s.y * color[1].r + s.z * color[2].r,
                    s.x * color[0].g + s.y * color[1].g + s.z * color[2].g,
                    s.x * color[0].b + s.y * color[1].b + s.z * color[2].b
                );
                // clang-format on
                c._ = to15bit(255 * calculatedColor.r, 255 * calculatedColor.g, 255 * calculatedColor.b);
            } else {
                // clang-format off
                glm::ivec2 calculatedTexel = glm::ivec2(
                    roundf(s.x * tex[0].x + s.y * tex[1].x + s.z * tex[2].x),
                    roundf(s.x * tex[0].y + s.y * tex[1].y + s.z * tex[2].y)
                );
                // clang-format on
                if (bits == 4) {
                    c = tex4bit(calculatedTexel, texPage, clut);
                } else if (bits == 8) {
                    c = tex8bit(calculatedTexel, texPage, clut);
                } else if (bits == 16) {
                    PSXColor raw = VRAM[texPage.y + calculatedTexel.y][texPage.x + calculatedTexel.x];
                    // TODO: I don't understand why this is necessary
                    // Without it PSOne BIOS display invalid colors under "Memory Card" and "CD Player"
                    c.r = raw.b;
                    c.g = raw.g;
                    c.b = raw.r;
                    c.k = raw.k;
                }
            }

            if ((bits != 0 || (flags & Vertex::SemiTransparency)) && c._ == 0x0000) continue;

            if (bits != 0 && !(flags & Vertex::RawTexture)) {
                c = c * color[0];
            }

            if (flags & Vertex::SemiTransparency && c.k) {
                GP0_E1::SemiTransparency transparency = (GP0_E1::SemiTransparency)((flags & 0xA0) >> 6);
                PSXColor bg = VRAM[p.y][p.x];
                if (transparency == GP0_E1::SemiTransparency::Bby2plusFby2) {
                    c = bg / 2.f + c / 2.f;
                } else if (transparency == GP0_E1::SemiTransparency::BplusF) {
                    c = bg + c;
                } else if (transparency == GP0_E1::SemiTransparency::BminusF) {
                    c = bg - c;
                } else if (transparency == GP0_E1::SemiTransparency::BplusFby4) {
                    c = bg + c / 4.f;
                }
            }

            VRAM[p.y][p.x] = c._;
        }
    }
}

void GPU::drawTriangle(Vertex v[3]) {
    glm::ivec2 pos[3];
    glm::vec3 color[3];
    glm::ivec2 texcoord[3];
    glm::ivec2 texpage;
    glm::ivec2 clut;
    int bits;
    int flags;
    for (int j = 0; j < 3; j++) {
        pos[j] = glm::ivec2(v[j].position[0], v[j].position[1]);
        color[j] = glm::vec3(v[j].color[0] / 255.f, v[j].color[1] / 255.f, v[j].color[2] / 255.f);
        texcoord[j] = glm::ivec2(v[j].texcoord[0], v[j].texcoord[1]);
    }
    texpage = glm::ivec2(v[0].texpage[0], v[0].texpage[1]);
    clut = glm::ivec2(v[0].clut[0], v[0].clut[1]);
    bits = v[0].bitcount;
    flags = v[0].flags;

    triangle(pos, color, texcoord, texpage, clut, bits, flags);
}

void GPU::rasterize() {
    for (int i = 0; i < renderList.size(); i += 3) {
        Vertex v[3];
        for (int j = 0; j < 3; j++) v[j] = renderList[i + j];

        drawTriangle(v);
    }
}
}
}
