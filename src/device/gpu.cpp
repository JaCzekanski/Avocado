#include "gpu.h"
#include <cstdio>
#include <SDL.h>

namespace device {
namespace gpu {
uint32_t colorMean(int *colors, int n) {
    int r = 0, g = 0, b = 0;
    for (int i = 0; i < n; i++) {
        b += (colors[i] & 0xff0000) >> 16;
        g += (colors[i] & 0xff00) >> 8;
        r += (colors[i] & 0xff);
    }
    b /= n;
    g /= n;
    r /= n;

    return 0xff << 24 | (r & 0xff) | (g & 0xff) << 8 | (b & 0xff) << 16;
}

inline int min(int a, int b, int c) {
    if (a < b) return (a < c) ? a : c;
    return (b < c) ? b : c;
}
inline int max(int a, int b, int c) {
    if (a > b) return (a > c) ? a : c;
    return (b > c) ? b : c;
}

void swap(int &a, int &b) {
    int t = b;
    b = a;
    a = t;
}

inline int distance(int x1, int y1, int x2, int y2) {
    return (int)sqrtf((float)((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)));
}

template <typename T>
inline T clamp(T number, int range) {
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

#define texX(x) ((!textured) ? 0 : (x & 0xff))
#define texY(x) ((!textured) ? 0 : ((x & 0xff00) >> 8))

    for (int i : {0, 1, 2}) {
        int r = c[i] & 0xff;
        int g = (c[i] >> 8) & 0xff;
        int b = (c[i] >> 16) & 0xff;
        renderList.push_back({x[i], y[i], r, g, b, texX(t[i]), texY(t[i]), bitcount, clutX, clutY, baseX, baseY});
    }

    if (isFourVertex) {
        for (int i : {1, 2, 3}) {
            int r = c[i] & 0xff;
            int g = (c[i] >> 8) & 0xff;
            int b = (c[i] >> 16) & 0xff;
            renderList.push_back({x[i], y[i], r, g, b, texX(t[i]), texY(t[i]), bitcount, clutX, clutY, baseX, baseY});
        }
    }

#undef texX
#undef texY
}

uint32_t to15bit(uint32_t color) {
    uint32_t newColor = 0;
    newColor |= (color & 0xf80000) >> 19;
    newColor |= (color & 0xf800) >> 6;
    newColor |= (color & 0xf8) << 7;
    return newColor;
}

uint32_t to24bit(uint16_t color) {
    uint32_t newColor = 0;
    // 00 RR GG BB
    // newColor |= (color & 0x7c00) >> 7;
    // newColor |= (color & 0x3e0) << 6;
    // newColor |= (color & 0x1f) << 13;

    //	newColor |= (color & 0x7c00) << 9;
    //	newColor |= (color & 0x3e0) << 6;
    //	newColor |= (color & 0x1f) << 3;

    // WTF?!
    newColor |= (color & 0x7c00) << 1;
    newColor |= (color & 0x3e0) >> 2;
    newColor |= (color & 0x1f) << 19;

    return newColor;
}

void GPU::step() {
    // Calculate GPUSTAT
    GPUSTAT = (gp0_e1._reg & 0x7FF) | (textureDisableAllowed << 15) | (setMaskWhileDrawing << 11)
              | (checkMaskBeforeDraw << 12)

              | (1 << 13) | ((uint8_t)gp1_08.reverseFlag << 14) | ((uint8_t)gp0_e1.textureDisable << 15)

              | ((uint8_t)displayDisable << 23) | ((uint8_t)irqAcknowledge << 24)

              | ((uint8_t)gp1_08.horizontalResolution2 << 16) | ((gp1_08._reg & 0x3f) << 17)

              | ((dmaDirection == 0 ? 0 : (dmaDirection == 1 ? 1 : (dmaDirection == 2 ? 1 : readyVramToCpu))) << 25)

              | (1 << 26)                           // Ready for DMA command
              | (readyVramToCpu << 27) | (1 << 28)  // Ready for receive DMA block
              | (dmaDirection << 29) | (odd << 31);
}

uint8_t GPU::read(uint32_t address) {
    if (address >= 0 && address < 4) {
        if (gpuReadMode == 0)
            return GPUREAD >> (address * 8);
        else if (gpuReadMode == 1) {
            static int write = 0;
            uint32_t word = 0;
            word |= VRAM[currY][currX];
            word |= VRAM[currY][currX + 1] << 16;
            if (++write == 4) {
                write = 0;
                currX += 2;

                if (currX >= endX) {
                    currX = startX;
                    if (++currY >= endY) {
                        gpuReadMode = 0;
                    }
                }
            }
            return word >> (address * 8);
        } else if (gpuReadMode == 2)
            return GPUREAD >> (address * 8);
    }
    if (address >= 4 && address < 8) {
        step();
        return GPUSTAT >> ((address - 4) * 8);
    }
    return 0;
}

void GPU::write(uint32_t address, uint8_t data) {
    if (address < 4) {
        tmpGP0 |= data << (whichGP0Byte * 8);

        if (++whichGP0Byte == 4) {
            writeGP0(tmpGP0);
            tmpGP0 = 0;
            whichGP0Byte = 0;
        }
        return;
    }
    if (address < 8) {
        tmpGP1 |= data << (whichGP1Byte * 8);

        if (++whichGP1Byte == 4) {
            writeGP1(tmpGP1);
            tmpGP1 = 0;
            whichGP1Byte = 0;
        }
    }
}

void GPU::writeGP0(uint32_t data) {
    static bool finished = true;
    static uint32_t command = 0;
    static uint32_t argument = 0;
    static uint32_t arguments[32];
    static int currentArgument = 0;
    static int argumentCount = 0;

    static bool isManyArguments = false;

    if (finished) {
        command = data >> 24;
        argument = data & 0xffffff;
        finished = true;
        argumentCount = 0;
        currentArgument = 0;
        isManyArguments = false;

        if (command == 0x00) {
        }  // NOP
        else if (command == 0x01) {
        }                            // Clear Cache
        else if (command == 0x02) {  // Fill rectangle
            argumentCount = 2;
            finished = false;
        } else if (command >= 0x20 && command < 0x40) {  // Polygons
            bool isTextureMapped = (command & 4) != 0;   // True - texcoords, false - no texcoords
            bool isFourVertex = (command & 8) != 0;      // True - 4 vertex, false - 3 vertex
            bool isShaded = (command & 0x10) != 0;       // True - Gouroud shading, false - flat shading

            // Because first color is in argument (cmd+arg, not args[])
            argumentCount
                = (isFourVertex ? 4 : 3) * (isTextureMapped ? 2 : 1) * (isShaded ? 2 : 1) - (isShaded ? 1 : 0);
            finished = false;
        } else if (command >= 0x40 && command < 0x60) {    // Lines
            isManyArguments = (command & 8) != 0;          // True - n points, false - 2 points (0x55555555 terminated)
            bool isShaded = ((command & 0x10) >> 4) != 0;  // True - Gouroud shading, false - flat shading

            argumentCount = (isManyArguments ? 0xff : (isShaded ? 2 : 1) * 2);
            finished = false;
        } else if (command >= 0x60 && command < 0x80) {  // Rectangles
            bool istextureMapped = (command & 4) != 0;   // True - texcoords, false - no texcoords
            int size = (command & 0x18) >> 3;            // 0 - free size, 1 - 1x1, 2 - 8x8, 3 - 16x16

            argumentCount = (size == 0 ? 2 : 1) + (istextureMapped ? 1 : 0);
            finished = false;
        } else if (command == 0xa0) {  // Copy rectangle (CPU -> VRAM)
            argumentCount = 2;
            finished = false;
        } else if (command == 0xc0) {  // Copy rectangle (VRAM -> CPU)
            readyVramToCpu = true;
            argumentCount = 2;
            finished = false;
        } else if (command == 0x80) {  // Copy rectangle (VRAM -> VRAM)
            argumentCount = 3;
            finished = false;
        } else if (command == 0xE1) {  // Draw mode setting
            gp0_e1._reg = argument;
        } else if (command == 0xe2) {  // Texture window setting
            textureWindowMaskX = argument & 0x1f;
            textureWindowMaskY = (argument & 0x3e0) >> 5;
            textureWindowOffsetX = (argument & 0x7c00) >> 10;
            textureWindowOffsetY = (argument & 0xf8000) >> 15;
        } else if (command == 0xe3) {  // Drawing area top left
            drawingAreaX1 = argument & 0x3ff;
            drawingAreaY1 = (argument & 0xFFC00) >> 10;
        } else if (command == 0xe4) {  // Drawing area bottom right
            drawingAreaX2 = argument & 0x3ff;
            drawingAreaY2 = (argument & 0xFFC00) >> 10;
        } else if (command == 0xe5) {  // Drawing offset
            drawingOffsetX = argument & 0x7ff;
            drawingOffsetY = (argument & 0x3FF800) >> 11;
        } else if (command == 0xe6) {  // Mask bit setting
            setMaskWhileDrawing = argument & 1;
            checkMaskBeforeDraw = (argument & 2) >> 1;
        } else
            printf("GP0(0x%02x) args 0x%06x\n", command, argument);

        return;
    }

    if (currentArgument < argumentCount) {
        arguments[currentArgument++] = data;
        if (isManyArguments && data == 0x55555555) argumentCount = currentArgument;
    }
    if (currentArgument != argumentCount) {
        return;
    }
    finished = true;
    if (command == 0x02) {  // fill rectangle
        currX = (arguments[0] & 0xffff);
        currY = (arguments[0] & 0xffff0000) > 16;
        startX = currX;
        endX = startX + (arguments[1] & 0xffff);
        startY = currY;
        endY = startY + ((arguments[1] & 0xffff0000) >> 16);

        uint32_t color = to15bit(argument & 0xffffff);

        for (;;) {
            if (currY < 512 && currX < 1023) {
                VRAM[currY][currX] = color;
            }

            if (currX++ >= endX) {
                currX = startX;
                if (++currY >= endY) break;
            }
        }
    } else if (command >= 0x20 && command < 0x40) {  // Polygons
        bool isTextureMapped = (command & 4) != 0;   // True - texcoords, false - no texcoords
        bool isFourVertex = (command & 8) != 0;      // True - 4 vertex, false - 3 vertex
        bool isShaded = (command & 0x10) != 0;       // True - Gouroud shading, false - flat shading

        int ptr = 0;
        int x[4], y[4];
        int color[4] = {0};
        int texcoord[4] = {0};
        for (int i = 0; i < (isFourVertex ? 4 : 3); i++) {
            x[i] = (arguments[ptr] & 0xffff);
            y[i] = ((arguments[ptr++] >> 16) & 0xffff);

            if (!isShaded || i == 0) color[i] = argument & 0xffffff;
            if (isTextureMapped) texcoord[i] = arguments[ptr++];
            if (isShaded && i < (isFourVertex ? 4 : 3) - 1) color[i + 1] = arguments[ptr++];
        }
        drawPolygon(x, y, color, texcoord, isFourVertex, isTextureMapped);
    } else if (command >= 0x40 && command < 0x60) {  // Lines
        bool isShaded = (command & 0x10) != 0;       // True - Gouroud shading, false - flat shading

        int ptr = 0;
        int sx, sy, sc;
        int ex = 0, ey = 0, ec = 0;
        for (int i = 0; i < argumentCount; i++) {
            if (i == 0) {
                sx = arguments[ptr] & 0xffff;
                sy = (arguments[ptr++] & 0xffff0000) >> 16;
                if (isShaded)
                    sc = arguments[ptr++];
                else
                    sc = command & 0xffffff;
            } else {
                sx = ex;
                sy = ey;
                sc = ec;
            }

            ex = arguments[ptr] & 0xffff;
            ey = (arguments[ptr++] & 0xffff0000) >> 16;
            if (isShaded)
                ec = arguments[ptr++];
            else
                ec = command & 0xffffff;

            int x[3] = {sx, sx, ex};
            int y[3] = {sy, sy, ey};
            int c[3] = {sc, sc, sc};

            drawPolygon(x, y, c);
        }
    } else if (command >= 0x60 && command < 0x80) {  // Rectangles
        bool isTextureMapped = (command & 4) != 0;   // True - texcoords, false - no texcoords
        int size = (command & 0x18) >> 3;            // 0 - free size, 1 - 1x1, 2 - 8x8, 3 - 16x16

        int w = 1;
        int h = 1;

        if (size == 1)
            w = h = 1;
        else if (size == 2)
            w = h = 8;
        else if (size == 3)
            w = h = 16;
        else {
            w = clamp(arguments[(isTextureMapped ? 2 : 1)] & 0xffff, 1023);
            h = clamp((arguments[(isTextureMapped ? 2 : 1)] & 0xffff0000) >> 16, 511);
        }
        int x = arguments[0] & 0xffff;
        int y = (arguments[0] & 0xffff0000) >> 16;

        int _x[4] = {x, x + w, x, x + w};
        int _y[4] = {y, y, y + h, y + h};
        int _c[4] = {(int)argument, (int)argument, (int)argument, (int)argument};
        int _t[4];

        if (isTextureMapped) {
#define tex(x, y) ((x & 0xff) | ((y & 0xff) << 8));
            int texX = arguments[1] & 0xff;
            int texY = (arguments[1] & 0xff00) >> 8;

            _t[0] = arguments[1];
            _t[1] = (gp0_e1._reg << 16) | tex(texX + w, texY);
            _t[2] = tex(texX, texY + h);
            _t[3] = tex(texX + w, texY + h);
#undef tex
        }

        drawPolygon(_x, _y, _c, _t, true, isTextureMapped);
    } else if (command == 0xA0) {  // Copy rectangle ( CPU -> VRAM )
        if (currentArgument <= 2) {
            argumentCount = 3;
            finished = false;
            startX = currX = clamp(arguments[0] & 0xffff, 1023);
            startY = currY = clamp((arguments[0] & 0xffff0000) >> 16, 511);

            endX = clamp(startX + (arguments[1] & 0xffff) - 1, 1023) + 1;
            endY = clamp(startY + ((arguments[1] & 0xffff0000) >> 16) - 1, 511) + 1;
        } else {
            currentArgument = 2;
            finished = false;
            uint32_t byte = arguments[2];

            // TODO: ugly code
            VRAM[currY][currX++] = byte & 0xffff;
            if (currX >= endX) {
                currX = startX;
                if (++currY >= endY) finished = true;
            }

            VRAM[currY][currX++] = (byte >> 16) & 0xffff;
            if (currX >= endX) {
                currX = startX;
                if (++currY >= endY) finished = true;
            }
        }
    } else if (command == 0xC0) {  // Copy rectangle ( VRAM -> CPU )
        finished = true;
        gpuReadMode = 1;
        startX = currX = clamp(arguments[0] & 0xffff, 1023);
        startY = currY = clamp((arguments[0] & 0xffff0000) >> 16, 511);
        endX = clamp(startX + (arguments[1] & 0xffff) - 1, 1023) + 1;
        endY = clamp(startY + ((arguments[1] & 0xffff0000) >> 16) - 1, 511) + 1;
    } else if (command == 0x80) {  // Copy rectangle ( VRAM -> VRAM )
        finished = true;
        int srcX = clamp(arguments[0] & 0xffff, 1023);
        int srcY = clamp((arguments[0] & 0xffff0000) >> 16, 511);

        int dstX = clamp(arguments[1] & 0xffff, 1023);
        int dstY = clamp((arguments[1] & 0xffff0000) >> 16, 511);

        int width = clamp((arguments[2] & 0xffff) - 1, 1023) + 1;
        int height = clamp((arguments[2] & 0xffff0000) >> 16 - 1, 511) + 1;

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                VRAM[dstY + y][dstX + x] = VRAM[srcY + y][srcX + x];
            }
        }
    }
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
        if (argument == 7) {                          // GPU Version
            GPUREAD = 2;
            gpuReadMode = 2;
        } else
            printf("Unimplemented GPU info request!\n");
    } else
        printf("GP1(0x%02x) args 0x%06x\n", command, argument);
}
std::vector<opengl::Vertex> &GPU::render() { return renderList; }
}
}