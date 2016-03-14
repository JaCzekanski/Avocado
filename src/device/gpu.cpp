#include "gpu.h"
#include <cstdio>
#include <SDL.h>
#include "../utils/file.h"
#include <cmath>

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
    return (int)sqrtf((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

void GPU::drawPolygon(int x[3], int y[3], int c[3]) {
    // Make ccw
    int r0 = ((c[0] & 0xff));
    int r1 = ((c[1] & 0xff));
    int r2 = ((c[2] & 0xff));

    int g0 = (((c[0] >> 8) & 0xff));
    int g1 = (((c[1] >> 8) & 0xff));
    int g2 = (((c[2] >> 8) & 0xff));

    int b0 = (((c[0] >> 16) & 0xff));
    int b1 = (((c[1] >> 16) & 0xff));
    int b2 = (((c[2] >> 16) & 0xff));

    int y1 = y[0];
    int y2 = y[1];
    int y3 = y[2];

    int x1 = x[0];
    int x2 = x[1];
    int x3 = x[2];

    // Bounding rectangle
    int minx = (int)min(x1, x2, x3);
    int maxx = (int)max(x1, x2, x3);
    int miny = (int)min(y1, y2, y3);
    int maxy = (int)max(y1, y2, y3);

    maxx = min(maxx, 640 - 1, 9999);
    maxy = min(maxy, 480 - 1, 9999);

    uint32_t *colorBuffer = (uint32_t *)pixels;

    (uint8_t *&)colorBuffer += miny * stride;

    // Scan through bounding rectangle
    for (int y = miny; y < maxy; y++) {
        for (int x = minx; x < maxx; x++) {
            // When all half-space functions positive, pixel is in triangle
            if ((x1 - x2) * (y - y1) - (y1 - y2) * (x - x1) > 0 && (x2 - x3) * (y - y2) - (y2 - y3) * (x - x2) > 0
                && (x3 - x1) * (y - y3) - (y3 - y1) * (x - x3) > 0) {
                int d1 = distance(x1, y1, x, y);
                int d2 = distance(x2, y2, x, y);
                int d3 = distance(x3, y3, x, y);
                float ds = (d1 + d2 + d3) / 3;

                int r = ((d1 / ds * r0) + (d2 / ds * r1) + (d3 / ds * r2)) / 3;

                int g = ((d1 / ds * g0) + (d2 / ds * g1) + (d3 / ds * g2)) / 3;

                int b = ((d1 / ds * b0) + (d2 / ds * b1) + (d3 / ds * b2)) / 3;

                colorBuffer[x] = 0xff000000 | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);
            }
        }
        (uint8_t *&)colorBuffer += stride;
    }
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
	//newColor |= (color & 0x7c00) >> 7;
	//newColor |= (color & 0x3e0) << 6;
	//newColor |= (color & 0x1f) << 13;

	newColor |= (color & 0x7c00) << 9;
	newColor |= (color & 0x3e0) << 6;
	newColor |= (color & 0x1f) << 3;

    return newColor;
}

void GPU::step() {
    // Calculate GPUSTAT
    GPUSTAT = (gp0_e1._reg & 0x7FF) | (textureDisableAllowed << 15) | (setMaskWhileDrawing << 11)
              | (checkMaskBeforeDraw << 12)

              | (1 << 13) | ((uint8_t)gp1_08.reverseFlag << 14) | ((uint8_t)gp0_e1.textureDisable << 15)

              | ((uint8_t)displayDisable << 23) | ((uint8_t)irqAcknowledge << 24)

              | ((uint8_t)gp1_08.horizontalResolution2 << 16) | ((gp1_08._reg & 0x3f) << 17)

			  | ((dmaDirection == 0 ? 0 : 
			   (dmaDirection == 1 ? 1 :
			   (dmaDirection == 2 ? 1 : readyVramToCpu))) << 25
			  )

              | (1 << 26)                           // Ready for DMA command
              | (readyVramToCpu << 27) 
			  | (1 << 28)  // Ready for receive DMA block
              | (dmaDirection << 29)
              | (odd << 31);
}

uint8_t GPU::read(uint32_t address) {
    if (address >= 0 && address < 4) {
        if (gpuReadMode == 0)
            return GPUREAD >> (address * 8);
        else if (gpuReadMode == 1) {
            static int write = 0;
            uint32_t word = 0;
            word |= VRAM[currY][currX];
            word |= VRAM[currY][currX+1] << 16;
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
    if (address >= 0 && address < 4) {
        tmpGP0 >>= 8;
        tmpGP0 |= data << 24;

        if (++whichGP0Byte == 4) {
            writeGP0(tmpGP0);
            tmpGP0 = 0;
            whichGP0Byte = 0;
        }
    } else if (address >= 4 && address < 8) {
        tmpGP1 >>= 8;
        tmpGP1 |= data << 24;

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
            bool istextureMapped = command & 4;          // True - texcoords, false - no texcoords
            bool isFourVertex = command & 8;             // True - 4 vertex, false - 3 vertex
            bool isShaded = command & 0x10;              // True - Gouroud shading, false - flat shading

            // Because first color is in argument (cmd+arg, not args[])
            argumentCount
                = (isFourVertex ? 4 : 3) * (istextureMapped ? 2 : 1) * (isShaded ? 2 : 1) - (isShaded ? 1 : 0);
            finished = false;
        } else if (command >= 0x40 && command < 0x60) {  // Lines
            isManyArguments = command & 8;               // True - n points, false - 2 points (0x55555555 terminated)
            bool isShaded = (command & 0x10) >> 4;       // True - Gouroud shading, false - flat shading

            argumentCount = (isManyArguments ? 0xff : (isShaded ? 2 : 1) * 2);
            finished = false;
        } else if (command >= 0x60 && command < 0x80) {  // Rectangles
            bool istextureMapped = command & 4;          // True - texcoords, false - no texcoords
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
                VRAM[currY][currX++] = color;
            }

            if (currX >= endX) {
                currX = startX;
                if (++currY >= endY) break;
            }
        }
    } else if (command >= 0x20 && command < 0x40) {  // Polygons
        bool isTextureMapped = command & 4;          // True - texcoords, false - no texcoords
        bool isFourVertex = command & 8;             // True - 4 vertex, false - 3 vertex
        bool isShaded = command & 0x10;              // True - Gouroud shading, false - flat shading

        int ptr = 0;
        int x[4], y[4];
        int color[4];
        int texcoord[4];
        for (int i = 0; i < (isFourVertex ? 4 : 3); i++) {
            x[i] = (arguments[ptr] & 0xffff);
            y[i] = ((arguments[ptr++] >> 16) & 0xffff);

            if (!isShaded || i == 0) color[i] = argument & 0xffffff;
            if (isTextureMapped) texcoord[i] = ptr++;
            if (isShaded && i < (isFourVertex ? 4 : 3) - 1) color[i + 1] = arguments[ptr++];
        }
        drawPolygon(x, y, color);
        if (isFourVertex) drawPolygon(x + 1, y + 1, color + 1);
        swap(x[0], x[1]);
        swap(y[0], y[1]);
        swap(color[0], color[1]);
        drawPolygon(x, y, color);
    } else if (command >= 0x40 && command < 0x60) {  // Lines
        bool isShaded = command & 0x10;              // True - Gouroud shading, false - flat shading

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

            // lineColor(renderer, sx, sy, ex, ey, colorMean(&ec, 1));
            int x[3] = {sx, sx, ex};
            int y[3] = {ex, ex, ey};
            int c[3] = {sc, sc, sc};

            drawPolygon(x, y, c);
        }
    } else if (command >= 0x60 && command < 0x80) {  // Rectangles
        bool istextureMapped = command & 4;          // True - texcoords, false - no texcoords
        int size = (command & 0x18) >> 3;            // 0 - free size, 1 - 1x1, 2 - 8x8, 3 - 16x16

        int w = 1;
        int h = 1;

        if (size == 2)
            w = h = 8;
        else if (size == 3)
            w = h = 16;
        else {
            w = (arguments[(istextureMapped ? 2 : 1)] & 0xffff);
            h = (arguments[(istextureMapped ? 2 : 1)] & 0xffff0000) >> 16;

            if (h > 1023) h = 1023;
            if (w > 511) w = 511;
        }
        int x = arguments[0] & 0xffff;
        int y = (arguments[0] & 0xffff0000) >> 16;

        int _x[4] = {x, x + w, x, x + w};
        int _y[4] = {y, y, y + h, y + h};
		int _c[3] = { (int)argument, (int)argument, (int)argument };

        drawPolygon(_x + 1, _y + 1, _c);
        swap(_x[0], _x[1]);
        swap(_y[0], _y[1]);
        drawPolygon(_x, _y, _c);
    } else if (command == 0xA0) {  // Copy rectangle ( CPU -> VRAM )
        if (currentArgument <= 2) {
            argumentCount = 3;
            finished = false;
            currX = (arguments[0] & 0xffff);
            currY = (arguments[0] & 0xffff0000) >> 16;

			if (currX >= 1024 - 2) currX = 1023 - 2;
			if (currY >= 512 - 2) currY = 511 - 2;

            startX = currX;
            endX = startX + (arguments[1] & 0xffff);
            startY = currY;
            endY = startY + ((arguments[1] & 0xffff0000) >> 16);

			if (endX >= 1024-2) endX = 1023-2;
			if (endY >= 512-2) endY = 511-2;
        } else {
            currentArgument = 2;
            finished = false;
            uint32_t byte = arguments[2];

            VRAM[currY][currX++] = byte&0xffff;
            VRAM[currY][currX++] = (byte >> 16) & 0xffff;

            if (currX >= endX) {
                currX = startX;
                if (++currY >= endY) finished = true;
            }
        }
    } else if (command == 0xC0) {  // Copy rectangle ( VRAM -> CPU )
        finished = true;
        gpuReadMode = 1;
        currX = (arguments[0] & 0xffff);
        currY = (arguments[0] & 0xffff0000) >> 16;
        startX = currX;
        endX = startX + (arguments[1] & 0xffff);
        startY = currY;
        endY = startY + ((arguments[1] & 0xffff0000) >> 16);
    } else if (command == 0x80) {  // Copy rectangle ( VRAM -> VRAM )
        finished = true;
        int srcX = (arguments[0] & 0xffff);
        int srcY = (arguments[0] & 0xffff0000) >> 16;

        int dstX = (arguments[1] & 0xffff);
        int dstY = (arguments[1] & 0xffff0000) >> 16;

        int width = (arguments[2] & 0xffff);
        int height = (arguments[2] & 0xffff0000) >> 16;

        int x = 0;
        int y = 0;

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (dstY + y >= 512 || dstX + x >= 1024 || srcY + y >= 512 || srcX + x >= 1024) {
                    continue;
                }
                VRAM[dstY + y][dstX + x] = VRAM[srcY + y][srcX + x];
            }
        }
    }
}

void GPU::writeGP1(uint32_t data) {
    uint32_t command = data >> 24;
    uint32_t argument = data & 0xffffff;

    if (command == 0x00) {  // Reset GPU

        irqAcknowledge = Bit::cleared;
        displayDisable = Bit::set;
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
        irqAcknowledge = Bit::cleared;
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
void GPU::render() {
    uint8_t *ptr = nullptr;
    int pitch;
    SDL_LockTexture(texture, NULL, &(void *&)ptr, &pitch);

    uint32_t col = 0;
    for (int y = 0; y < 512; y++) {
        int x = 0;
        for (int p = 0; p < pitch; p++) {
            if (p == 0 || (p % 4) == 0) {
				// 00RRGGBB
				col = to24bit(VRAM[y][x++]);
            }
            *(ptr++) = col;
            col >>= 8;
        }
    }
    SDL_UnlockTexture(texture);

    SDL_UnlockTexture(SCREEN);
    SDL_RenderCopy(renderer, SCREEN, NULL, NULL);

    SDL_Rect dst = {320, 0, 320, 240};
    SDL_RenderCopy(renderer, texture, NULL, &dst);

    SDL_RenderPresent(renderer);
    SDL_LockTexture(SCREEN, NULL, &pixels, &stride);
}
}
}