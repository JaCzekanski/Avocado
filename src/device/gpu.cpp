#include "gpu.h"
#include <cstdio>
#include <cmath>

namespace device {
namespace gpu {
template <typename T>
T clamp(T number, size_t range) {
    if (number > range) number = range;
    return number;
}

void GPU::drawPolygon(int x[4], int y[4], int c[4], int t[4], bool isFourVertex, bool textured) {
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
        clutX = ((t[0] & 0x003f0000) >> 16) * 16;
        clutY = ((t[0] & 0xffc00000) >> 22);

        baseX = ((t[1] & 0x0f0000) >> 16) * 64;   // N * 64
        baseY = ((t[1] & 0x100000) >> 20) * 256;  // N* 256

        int depth = (t[1] & 0x1800000) >> 23;
        if (depth == 0) bitcount = 4;
        if (depth == 1) bitcount = 8;
        if (depth == 2) bitcount = 16;
    }

#define texX(x) ((!textured) ? 0 : ((x)&0xff))
#define texY(x) ((!textured) ? 0 : (((x)&0xff00) >> 8))

    for (int i : {0, 1, 2}) {
        int r = c[i] & 0xff;
        int g = (c[i] >> 8) & 0xff;
        int b = (c[i] >> 16) & 0xff;
        renderList.push_back({{x[i], y[i]}, {r, g, b}, {texX(t[i]), texY(t[i])}, bitcount, {clutX, clutY}, {baseX, baseY}});
    }

    if (isFourVertex) {
        for (int i : {1, 2, 3}) {
            int r = c[i] & 0xff;
            int g = (c[i] >> 8) & 0xff;
            int b = (c[i] >> 16) & 0xff;
            renderList.push_back({{x[i], y[i]}, {r, g, b}, {texX(t[i]), texY(t[i])}, bitcount, {clutX, clutY}, {baseX, baseY}});
        }
    }

#undef texX
#undef texY
}

void GPU::cmdFillRectangle(const uint8_t command, uint32_t arguments[]) {
    startX = currX = (arguments[1] & 0xffff);
    startY = currY = (arguments[1] & 0xffff0000) >> 16;
    endX = startX + (arguments[2] & 0xffff);
    endY = startY + ((arguments[2] & 0xffff0000) >> 16);

    uint32_t color = to15bit(arguments[0] & 0xffffff);

    for (;;) {
        if (currY < 512 && currX < 1023) {
            VRAM[currY][currX] = color;
        }

        if (currX++ >= endX) {
            currX = startX;
            if (++currY >= endY) break;
        }
    }

    cmd = Command::None;
}

void GPU::cmdPolygon(const PolygonArgs arg, uint32_t arguments[]) {
    int ptr = 1;
    int x[4], y[4], c[4] = {0}, tex[4] = {0};
    for (int i = 0; i < arg.getVertexCount(); i++) {
        x[i] = arguments[ptr] & 0xffff;
        y[i] = (arguments[ptr++] >> 16) & 0xffff;

        if (!arg.isShaded || i == 0) c[i] = arguments[0] & 0xffffff;
        if (arg.isTextureMapped) tex[i] = arguments[ptr++];
        if (arg.isShaded && i < arg.getVertexCount() - 1) c[i + 1] = arguments[ptr++];
    }
    drawPolygon(x, y, c, tex, arg.isQuad, arg.isTextureMapped);

    cmd = Command::None;
}

void GPU::cmdLine(const LineArgs arg, uint32_t arguments[]) {
    int ptr = 1;
    int sx = 0, sy = 0, sc = 0;
    int ex = 0, ey = 0, ec = 0;
    for (int i = 0; i < arg.getArgumentCount() - 1; i++) {
        if (i == 0) {
            sx = arguments[ptr] & 0xffff;
            sy = (arguments[ptr++] & 0xffff0000) >> 16;
            sc = arguments[0] & 0xffffff;
        } else {
            sx = ex;
            sy = ey;
            sc = ec;
        }

        if (arg.isShaded)
            ec = arguments[ptr++];
        else
            ec = arguments[0] & 0xffffff;
        ex = arguments[ptr] & 0xffff;
        ey = (arguments[ptr++] & 0xffff0000) >> 16;

        int x[3] = {sx, sx + 1, ex};
        int y[3] = {sy, sy + 1, ey};
        int c[3] = {sc, sc, sc};

        drawPolygon(x, y, c);
    }

    cmd = Command::None;
}

void GPU::cmdRectangle(const RectangleArgs arg, uint32_t arguments[]) {
    int w = arg.getSize();
    int h = arg.getSize();

    if (arg.size == 0) {
        w = clamp(arguments[(arg.isTextureMapped ? 3 : 2)] & 0xffff, 1023);
        h = clamp((arguments[(arg.isTextureMapped ? 3 : 2)] & 0xffff0000) >> 16, 511);
    }

    int x = arguments[1] & 0xffff;
    int y = (arguments[1] & 0xffff0000) >> 16;

    int _x[4] = {x, x + w, x, x + w};
    int _y[4] = {y, y, y + h, y + h};
    int _c[4] = {(int)arguments[0], (int)arguments[0], (int)arguments[0], (int)arguments[0]};
    int _t[4];

    if (arg.isTextureMapped) {
#define tex(x, y) (((x)&0xff) | (((y)&0xff) << 8));
        int texX = arguments[2] & 0xff;
        int texY = (arguments[2] & 0xff00) >> 8;

        _t[0] = arguments[2];
        _t[1] = (gp0_e1._reg << 16) | tex(texX + w, texY);
        _t[2] = tex(texX, texY + h);
        _t[3] = tex(texX + w, texY + h);
#undef tex
    }

    drawPolygon(_x, _y, _c, _t, true, arg.isTextureMapped);

    cmd = Command::None;
}

void GPU::cmdCpuToVram1(const uint8_t command, uint32_t arguments[]) {
    startX = currX = clamp(arguments[1] & 0xffff, 1023);
    startY = currY = clamp((arguments[1] & 0xffff0000) >> 16, 511);

    endX = clamp(startX + (arguments[2] & 0xffff) - 1, 1023) + 1;
    endY = clamp(startY + ((arguments[2] & 0xffff0000) >> 16) - 1, 511) + 1;

    cmd = Command::CopyCpuToVram2;
    argumentCount = 1;
    currentArgument = 0;
}

void GPU::cmdCpuToVram2(const uint8_t command, uint32_t arguments[]) {
    uint32_t byte = arguments[0];

    // TODO: ugly code
    VRAM[currY][currX++] = byte & 0xffff;
    if (currX >= endX) {
        currX = startX;
        if (++currY >= endY) cmd = Command::None;
    }

    VRAM[currY][currX++] = (byte >> 16) & 0xffff;
    if (currX >= endX) {
        currX = startX;
        if (++currY >= endY) cmd = Command::None;
    }

    currentArgument = 0;
}

void GPU::cmdVramToCpu(const uint8_t command, uint32_t arguments[]) {
    gpuReadMode = 1;
    startX = currX = clamp(arguments[1] & 0xffff, 1023);
    startY = currY = clamp((arguments[1] & 0xffff0000) >> 16, 511);
    endX = clamp(startX + (arguments[2] & 0xffff) - 1, 1023) + 1;
    endY = clamp(startY + ((arguments[2] & 0xffff0000) >> 16) - 1, 511) + 1;

    cmd = Command::None;
}

void GPU::cmdVramToVram(const uint8_t command, uint32_t arguments[]) {
    int srcX = clamp(arguments[1] & 0xffff, 1023);
    int srcY = clamp((arguments[1] & 0xffff0000) >> 16, 511);

    int dstX = clamp(arguments[2] & 0xffff, 1023);
    int dstY = clamp((arguments[2] & 0xffff0000) >> 16, 511);

    int width = clamp((arguments[3] & 0xffff) - 1, 1023) + 1;
    int height = clamp(((arguments[3] & 0xffff0000) >> 16) - 1, 511) + 1;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            VRAM[dstY + y][dstX + x] = VRAM[srcY + y][srcX + x];
        }
    }

    cmd = Command::None;
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
    // Calculate GPUSTAT
    GPUSTAT = (gp0_e1._reg & 0x7FF) | (setMaskWhileDrawing << 11) | (checkMaskBeforeDraw << 12) | (1 << 13)  // always set
              | ((uint8_t)gp1_08.reverseFlag << 14) | ((uint8_t)gp0_e1.textureDisable << 15) | ((uint8_t)gp1_08.horizontalResolution2 << 16)
              | ((uint8_t)gp1_08.horizontalResolution1 << 17) | ((uint8_t)gp1_08.verticalResolution << 19)
              | ((uint8_t)gp1_08.videoMode << 20) | ((uint8_t)gp1_08.colorDepth << 21) | (gp1_08.interlace << 22)
              | ((uint8_t)displayDisable << 23) | ((uint8_t)irqAcknowledge << 24)
              | ((dmaDirection == 0 ? 0 : (dmaDirection == 1 ? 1 : (dmaDirection == 2 ? 1 : (cmd != Command::CopyCpuToVram2)))) << 25)
              | (1 << 26)                                             // Ready for DMA command
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
        } else if (command == 0xE1) {
            // Draw mode setting
            gp0_e1._reg = arguments[0];
        } else if (command == 0xe2) {
            // Texture window setting
            textureWindowMaskX = arguments[0] & 0x1f;
            textureWindowMaskY = (arguments[0] & 0x3e0) >> 5;
            textureWindowOffsetX = (arguments[0] & 0x7c00) >> 10;
            textureWindowOffsetY = (arguments[0] & 0xf8000) >> 15;
        } else if (command == 0xe3) {
            // Drawing area top left
            drawingAreaX1 = arguments[0] & 0x3ff;
            drawingAreaY1 = (arguments[0] & 0xFFC00) >> 10;
        } else if (command == 0xe4) {
            // Drawing area bottom right
            drawingAreaX2 = arguments[0] & 0x3ff;
            drawingAreaY2 = (arguments[0] & 0xFFC00) >> 10;
        } else if (command == 0xe5) {
            // Drawing offset
            drawingOffsetX = arguments[0] & 0x7ff;
            drawingOffsetY = (arguments[0] & 0x3FF800) >> 11;
        } else if (command == 0xe6) {
            // Mask bit setting
            setMaskWhileDrawing = arguments[0] & 1;
            checkMaskBeforeDraw = (arguments[0] & 2) >> 1;
        } else
            printf("GP0(0x%02x) args 0x%06x\n", command, arguments[0]);

        argumentCount++;
        return;
    }

    if (currentArgument < argumentCount) {
        arguments[currentArgument++] = data;
        // TODO: test multiline rendering
        if (argumentCount == MAX_ARGS && data == 0x55555555) argumentCount = currentArgument;
        if (currentArgument != argumentCount) return;
    }

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
    uint32_t command = data >> 24;
    uint32_t argument = data & 0xffffff;

    if (command == 0x00) {  // Reset GPU
        irqAcknowledge = false;
        displayDisable = true;
        dmaDirection = 0;
        displayAreaStartX = displayAreaStartY = 0;
        displayRangeX1 = 0x200;
        displayRangeX2 = 0x200 + 256 * 10;
        displayRangeY1 = 0x10;
        displayRangeY2 = 0x10 + 240;

        gp1_08._reg = 0;
        gp0_e1._reg = 0;

        textureWindowMaskX = 0;
        textureWindowMaskY = 0;
        textureWindowOffsetX = 0;
        textureWindowOffsetY = 0;

        drawingAreaX1 = 0;
        drawingAreaY1 = 0;

        drawingAreaX2 = 0;
        drawingAreaY2 = 0;

        drawingOffsetX = 0;
        drawingOffsetY = 0;

        setMaskWhileDrawing = 0;
        checkMaskBeforeDraw = 0;
    } else if (command == 0x01) {  // Reset command buffer

    } else if (command == 0x02) {  // Acknowledge IRQ1
        irqAcknowledge = false;
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

        if (argument == 3)
            GPUREAD = (drawingAreaY1 << 10) | drawingAreaX1;
        else if (argument == 4)
            GPUREAD = (drawingAreaY2 << 10) | drawingAreaX2;
        else if (argument == 5)
            GPUREAD = (drawingOffsetY << 11) | drawingOffsetX;
        else if (argument == 7)
            GPUREAD = 2;  // GPU Version
        else
            printf("Unimplemented GPU info request (arg: %d)!\n", argument);
    } else
        printf("GP1(0x%02x) args 0x%06x\n", command, argument);
}
std::vector<Vertex> &GPU::render() { return renderList; }
}
}
