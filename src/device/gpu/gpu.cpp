#include "gpu.h"
#include <fmt/core.h>
#include <cassert>
#include "config.h"
#include "render/render.h"
#include "system.h"
#include "utils/file.h"
#include "utils/logic.h"
#include "utils/macros.h"
#include "utils/timing.h"

// For vram dump
#include <stb_image_write.h>

namespace gpu {
GPU::GPU(System* sys) : sys(sys) {
    busToken = bus.listen<Event::Config::Graphics>([&](auto) { reload(); });
    reload();
    reset();
}

GPU::~GPU() { bus.unlistenAll(busToken); }

void GPU::reload() {
    verbose = config.debug.log.gpu;
    forceNtsc = config.options.graphics.forceNtsc;
    auto mode = config.options.graphics.renderingMode;
    softwareRendering = (mode & RenderingMode::software) != 0;
    hardwareRendering = (mode & RenderingMode::hardware) != 0;
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

    drawingArea = Rect<int16_t>();

    drawingOffsetX = 0;
    drawingOffsetY = 0;

    gp0_e6._reg = 0;

    clutCachePos = ivec2(-1, -1);
}

void GPU::drawTriangle(const primitive::Triangle& triangle) {
    if (hardwareRendering) {
        int flags = 0;
        if (triangle.isRawTexture) flags |= Vertex::Flags::RawTexture;
        if (gp0_e1.dither24to15) flags |= Vertex::Flags::Dithering;
        if (triangle.gouraudShading) flags |= Vertex::Flags::GouraudShading;
        if (triangle.isSemiTransparent) {
            flags |= Vertex::Flags::SemiTransparency;
            flags |= static_cast<int>(triangle.transparency) << 5;
        }

        for (int i : {0, 1, 2}) {
            auto& v = triangle.v[i];
            vertices.push_back({
                {static_cast<float>(v.pos.x), static_cast<float>(v.pos.y)},
                {v.color.r, v.color.g, v.color.b},
                {v.uv.x, v.uv.y},
                triangle.bits,
                {triangle.clut.x, triangle.clut.y},
                {triangle.texpage.x, triangle.texpage.y},
                flags,
                gp0_e2,
                gp0_e6,
            });
        }
    }

    if (softwareRendering) {
        Render::drawTriangle(this, triangle);
    }
}

void GPU::drawLine(const primitive::Line& line) {
    if (hardwareRendering) {
        vec2 p[2]{line.pos[0], line.pos[1]};
        auto c = line.color;
        int flags = 0;

        if (line.gouraudShading) flags |= Vertex::Flags::GouraudShading;
        if (line.isSemiTransparent) {
            flags |= Vertex::Flags::SemiTransparency;
            flags |= static_cast<int>(gp0_e1.semiTransparency) << 5;
        }

        // Calculate vector b perpendicular to p0p1 line
        vec2 angle(p[1] - p[0]);
        // Rotate 90°, normalize and make it half size
        vec2 b = vec2::normalize(vec2(angle.y, -angle.x)) / 2.f;

        auto pushVertex = [&](float x, float y, const RGB c) {
            vertices.push_back({
                {x, y},
                {c.r, c.g, c.b},
                {0, 0},  // UV: 0
                0,       // Bits: 0
                {0, 0},  // clut: 0
                {0, 0},  // texPage: 0
                flags,
                gp0_e2,
                gp0_e6,
            });
        };

        // Triangulate line
        pushVertex(p[0].x - b.x, p[0].y - b.y, c[0]);
        pushVertex(p[0].x + b.x, p[0].y + b.y, c[0]);
        pushVertex(p[1].x - b.x, p[1].y - b.y, c[1]);
        pushVertex(p[0].x + b.x, p[0].y + b.y, c[0]);
        pushVertex(p[1].x + b.x, p[1].y + b.y, c[1]);
        pushVertex(p[1].x - b.x, p[1].y - b.y, c[1]);
    }

    if (softwareRendering) {
        Render::drawLine(this, line);
    }
}

void GPU::drawRectangle(const primitive::Rect& rect) {
    if (hardwareRendering) {
        ivec2 p;
        float x[4], y[4];
        ivec2 uv[4];

        p.x = rect.pos.x;
        p.y = rect.pos.y;

        x[0] = p.x;
        y[0] = p.y;

        x[1] = p.x + rect.size.x;
        y[1] = p.y;

        x[2] = p.x;
        y[2] = p.y + rect.size.y;

        x[3] = p.x + rect.size.x;
        y[3] = p.y + rect.size.y;

        uv[0].x = rect.uv.x;
        uv[0].y = rect.uv.y;

        uv[1].x = rect.uv.x + rect.size.x;
        uv[1].y = rect.uv.y;

        uv[2].x = rect.uv.x;
        uv[2].y = rect.uv.y + rect.size.y;

        uv[3].x = rect.uv.x + rect.size.x;
        uv[3].y = rect.uv.y + rect.size.y;

        int flags = 0;
        if (rect.isRawTexture) flags |= Vertex::Flags::RawTexture;
        if (rect.isSemiTransparent) {
            flags |= Vertex::Flags::SemiTransparency;
            flags |= static_cast<int>(gp0_e1.semiTransparency) << 5;
        }

        Vertex v[6];
        for (int i : {0, 1, 2, 1, 2, 3}) {
            v[i] = {
                {x[i], y[i]},
                {rect.color.r, rect.color.g, rect.color.b},
                {uv[i].x, uv[i].y},
                rect.bits,
                {rect.clut.x, rect.clut.y},
                {rect.texpage.x, rect.texpage.y},
                flags,
                gp0_e2,
                gp0_e6,
            };
            vertices.push_back(v[i]);
        }
    }

    if (softwareRendering) {
        Render::drawRectangle(this, rect);
    }
}

void GPU::cmdFillRectangle() {
    struct mask {
        constexpr static int startX(int x) { return x & 0x3f0; }
        constexpr static int startY(int y) { return y & 0x1ff; }
        constexpr static int endX(int x) { return ((x & 0x3ff) + 0x0f) & ~0x0f; }
        constexpr static int endY(int y) { return y & 0x1ff; }
    };

    startX = std::max<int>(0, mask::startX(arguments[1] & 0xffff));
    startY = std::max<int>(0, mask::startY((arguments[1] & 0xffff0000) >> 16));
    endX = std::min<int>(VRAM_WIDTH, startX + mask::endX(arguments[2] & 0xffff));
    endY = std::min<int>(VRAM_HEIGHT, startY + mask::endY((arguments[2] & 0xffff0000) >> 16));

    uint32_t color = to15bit(arguments[0] & 0xffffff);

    // Note: not sure if coords should include last column and row
    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            VRAM[y][x] = color;
        }
    }

    cmd = Command::None;

    if (hardwareRendering) {
        ivec2 p[4] = {
            {startX, startY},
            {endX, startY},
            {startX, endY},
            {endX, endY},
        };

        RGB c;
        c.raw = arguments[0];

        GP0_E2 e2;
        GP0_E6 e6;

        Vertex v[6];
        for (int i : {0, 1, 2, 1, 2, 3}) {
            v[i] = {
                {static_cast<float>(p[i].x), static_cast<float>(p[i].y)},
                {c.r, c.g, c.b},
                {0, 0},  // UV: 0
                0,       // Bits: 0
                {0, 0},  // clut: 0
                {0, 0},  // texPage: 0
                0,       // Flags: 0
                e2,      // gp0_e2: 0
                e6,      // gp0_e6: 0, no mask
            };
            vertices.push_back(v[i]);
        }
    }
}

void GPU::cmdPolygon(PolygonArgs arg) {
    int ptr = 1;

    primitive::Triangle::Vertex v[4];
    TextureInfo tex;

    for (int i = 0; i < arg.getVertexCount(); i++) {
        v[i].pos.x = drawingOffsetX + extend_sign<11>(arguments[ptr] & 0xffff);
        v[i].pos.y = drawingOffsetY + extend_sign<11>((arguments[ptr++] & 0xffff0000) >> 16);

        // If flat shaded - use first color for all vertices.
        // Otherwise - apply only for the first vertex.
        if (!arg.gouraudShading || i == 0) v[i].color.raw = arguments[0] & 0xffffff;
        if (arg.isTextureMapped) {
            if (i == 0) tex.palette = arguments[ptr];
            if (i == 1) tex.texpage = arguments[ptr];
            v[i].uv.x = arguments[ptr] & 0xff;
            v[i].uv.y = (arguments[ptr] >> 8) & 0xff;
            ptr++;
        }
        if (arg.gouraudShading && i < arg.getVertexCount() - 1) v[i + 1].color.raw = arguments[ptr++] & 0xffffff;
    }

    primitive::Triangle triangle;

    for (int i : {0, 1, 2}) triangle.v[i] = v[i];

    triangle.bits = 0;
    triangle.isSemiTransparent = arg.semiTransparency;
    triangle.transparency = gp0_e1.semiTransparency;
    triangle.isRawTexture = arg.isRawTexture;
    triangle.gouraudShading = arg.gouraudShading;

    if (arg.isTextureMapped) {
        triangle.bits = tex.getBitcount();
        triangle.texpage.x = tex.getBaseX();
        triangle.texpage.y = tex.getBaseY();
        triangle.clut.x = tex.getClutX();
        triangle.clut.y = tex.getClutY();
        triangle.transparency = tex.semiTransparencyBlending();

        // Copy texture bits from Polygon draw command to E1 register
        // texpagex, texpagey, semi-transparency, texture disable bits
        const uint32_t E1_MASK = 0b00001001'11111111;

        uint16_t newBits = (tex.texpage >> 16) & E1_MASK;
        if (!textureDisableAllowed) {
            newBits &= ~(1 << 11);
        }

        gp0_e1._reg &= ~E1_MASK;
        gp0_e1._reg |= newBits;
    }

    triangle.assureCcw();
    drawTriangle(triangle);

    if (arg.isQuad) {
        for (int i : {1, 2, 3}) triangle.v[i - 1] = v[i];

        triangle.assureCcw();
        drawTriangle(triangle);
    }

    cmd = Command::None;
}

void GPU::cmdLine(LineArgs arg) {
    int ptr = 1;

    primitive::Line line;
    line.isSemiTransparent = arg.semiTransparency;
    line.gouraudShading = arg.gouraudShading;

    line.pos[0].x = drawingOffsetX + extend_sign<11>(arguments[ptr] & 0xffff);
    line.pos[0].y = drawingOffsetY + extend_sign<11>((arguments[ptr++] & 0xffff0000) >> 16);
    line.color[0].raw = (arguments[0] & 0xffffff);

    if (arg.gouraudShading) {
        line.color[1].raw = arguments[ptr++];
    } else {
        line.color[1] = line.color[0];
    }

    line.pos[1].x = drawingOffsetX + extend_sign<11>((arguments[ptr] & 0xffff));
    line.pos[1].y = drawingOffsetY + extend_sign<11>((arguments[ptr++] & 0xffff0000) >> 16);

    drawLine(line);

    if (!arg.polyLine) {
        cmd = Command::None;
        return;
    }

    // Swap pos[1] to pos[0] (and color if shaded), so that next lines can be rendered
    if (!arg.gouraudShading) {
        arguments[1] = arguments[2];  // Pos
    } else {
        arguments[1] = arguments[3];  // Pos
        arguments[0] = arguments[2];  // Color
    }
    currentArgument = 2;
}

void GPU::cmdRectangle(RectangleArgs arg) {
    /* Rectangle command format
    arg[0] = cmd + color      (0xCCBB GGRR)
    arg[1] = pos              (0xYYYY XXXX)
    arg[2] = Palette + tex UV (0xCLUT VVUU) (*if textured)
    arg[3] = size             (0xHHHH WWWW) (*if variable size) */
    using Bits = gpu::GP0_E1::TexturePageColors;

    int16_t w = arg.getSize();
    int16_t h = arg.getSize();

    if (arg.size == 0) {
        w = extend_sign<11>(arguments[(arg.isTextureMapped ? 3 : 2)] & 0xffff);
        h = extend_sign<11>((arguments[(arg.isTextureMapped ? 3 : 2)] & 0xffff0000) >> 16);
    }

    int16_t x = drawingOffsetX + extend_sign<11>(arguments[1] & 0xffff);
    int16_t y = drawingOffsetY + extend_sign<11>((arguments[1] & 0xffff0000) >> 16);

    primitive::Rect rect;
    rect.pos = ivec2(x, y);
    rect.size = ivec2(w, h);
    rect.color.raw = (arguments[0] & 0xffffff);

    if (!arg.isTextureMapped) {
        rect.bits = 0;
    } else if (gp0_e1.texturePageColors == Bits::bit4) {
        rect.bits = 4;
    } else if (gp0_e1.texturePageColors == Bits::bit8) {
        rect.bits = 8;
    } else if (gp0_e1.texturePageColors == Bits::bit15) {
        rect.bits = 16;
    } else if (gp0_e1.texturePageColors == Bits::reserved) {
        rect.bits = 16;
    }
    rect.isSemiTransparent = arg.semiTransparency;
    rect.isRawTexture = arg.isRawTexture;

    if (arg.isTextureMapped) {
        union Argument2 {
            struct {
                uint32_t u : 8;
                uint32_t v : 8;
                uint32_t clutX : 6;
                uint32_t clutY : 9;
                uint32_t : 1;
            };
            uint32_t _raw;
            Argument2(uint32_t arg) : _raw(arg) {}
        };
        Argument2 p = arguments[2];

        rect.uv = ivec2(p.u, p.v);
        rect.clut = ivec2(p.clutX * 16, p.clutY);
        rect.texpage = ivec2(gp0_e1.texturePageBaseX * 64, gp0_e1.texturePageBaseY * 256);
    }

    drawRectangle(rect);

    cmd = Command::None;
}

struct MaskCopy {
    constexpr static int x(int x) { return x & 0x3ff; }
    constexpr static int y(int y) { return y & 0x1ff; }
    constexpr static int w(int w) { return ((w - 1) & 0x3ff) + 1; }
    constexpr static int h(int h) { return ((h - 1) & 0x1ff) + 1; }
};

void GPU::cmdCpuToVram1() {
    startX = currX = MaskCopy::x(arguments[1] & 0xffff);
    startY = currY = MaskCopy::y((arguments[1] & 0xffff0000) >> 16);

    endX = startX + MaskCopy::w(arguments[2] & 0xffff);
    endY = startY + MaskCopy::h((arguments[2] & 0xffff0000) >> 16);

    cmd = Command::CopyCpuToVram2;
    argumentCount = 1;
    currentArgument = 0;
}

void GPU::maskedWrite(int x, int y, uint16_t value) {
    uint16_t mask = gp0_e6.setMaskWhileDrawing << 15;
    x %= VRAM_WIDTH;
    y %= VRAM_HEIGHT;

    if (unlikely(gp0_e6.checkMaskBeforeDraw)) {
        if (VRAM[y][x] & 0x8000) return;
    }

    VRAM[y][x] = value | mask;
}

void GPU::cmdCpuToVram2() {
    const auto advanceOrBreak = [&]() {
        if (++currX >= endX) {
            currX = startX;
            if (++currY >= endY) {
                cmd = Command::None;
                return true;
            }
        }
        return false;
    };

    uint32_t value = arguments[0];
    currentArgument = 0;

    maskedWrite(currX, currY, value & 0xffff);
    if (advanceOrBreak()) return;

    maskedWrite(currX, currY, (value >> 16) & 0xffff);
    if (advanceOrBreak()) return;
}

void GPU::cmdVramToCpu() {
    readMode = ReadMode::Vram;
    startX = currX = MaskCopy::x(arguments[1] & 0xffff);
    startY = currY = MaskCopy::y((arguments[1] & 0xffff0000) >> 16);
    endX = startX + MaskCopy::w(arguments[2] & 0xffff);
    endY = startY + MaskCopy::h((arguments[2] & 0xffff0000) >> 16);

    cmd = Command::None;
}

uint32_t GPU::readVramData() {
    const auto advanceOrBreak = [&]() {
        if (++currX >= endX) {
            currX = startX;
            if (++currY >= endY) {
                readMode = ReadMode::Register;
            }
        }
    };

    uint32_t data = 0;

    data |= VRAM[currY % VRAM_HEIGHT][currX % VRAM_WIDTH];
    advanceOrBreak();
    data |= VRAM[currY % VRAM_HEIGHT][currX % VRAM_WIDTH] << 16;
    advanceOrBreak();

    return data;
}

void GPU::cmdVramToVram() {
    cmd = Command::None;

    int srcX = MaskCopy::x(arguments[1] & 0xffff);
    int srcY = MaskCopy::y((arguments[1] & 0xffff0000) >> 16);

    int dstX = MaskCopy::x(arguments[2] & 0xffff);
    int dstY = MaskCopy::y((arguments[2] & 0xffff0000) >> 16);

    int w = MaskCopy::w(arguments[3] & 0xffff);
    int h = MaskCopy::h((arguments[3] & 0xffff0000) >> 16);

    // Note: VramToVram copy is always Top-to-Bottom
    // but it might be Left-to-Right or Right-to-Left depending whether srcX < dstX
    // See gpu/vram-to-vram-overlap test
    bool dir = srcX < dstX;

    for (int y = 0; y < h; y++) {
        for (int _x = 0; _x < w; _x++) {
            int x = (!dir) ? _x : w - 1 - _x;

            uint16_t src = VRAM[(srcY + y) % VRAM_HEIGHT][(srcX + x) % VRAM_WIDTH];
            maskedWrite(dstX + x, dstY + y, src);
        }
    }
}

uint32_t GPU::getStat() {
    uint32_t GPUSTAT = 0;
    uint8_t dataRequest = 0;
    if (dmaDirection == 0)
        dataRequest = 0;
    else if (dmaDirection == 1)
        dataRequest = 1;  // FIFO not full
    else if (dmaDirection == 2)
        dataRequest = 1;  // Same as bit28, ready to receive dma block
    else if (dmaDirection == 3)
        dataRequest = readMode == ReadMode::Vram;  // Same as bit27, ready to send VRAM to CPU

    GPUSTAT = gp0_e1._reg & 0x7FF;
    GPUSTAT |= gp0_e6.setMaskWhileDrawing << 11;
    GPUSTAT |= gp0_e6.checkMaskBeforeDraw << 12;
    GPUSTAT |= 1 << 13;  // always set
    GPUSTAT |= (uint8_t)gp1_08.reverseFlag << 14;
    GPUSTAT |= (uint8_t)gp0_e1.textureDisable << 15;
    GPUSTAT |= (uint8_t)gp1_08.horizontalResolution2 << 16;
    GPUSTAT |= (uint8_t)gp1_08.horizontalResolution1 << 17;
    GPUSTAT |= (uint8_t)gp1_08.verticalResolution << 19;
    GPUSTAT |= (uint8_t)gp1_08.videoMode << 20;
    GPUSTAT |= (uint8_t)gp1_08.colorDepth << 21;
    GPUSTAT |= gp1_08.interlace << 22;
    GPUSTAT |= displayDisable << 23;
    GPUSTAT |= irqRequest << 24;
    GPUSTAT |= dataRequest << 25;
    GPUSTAT |= (cmd == Command::None) << 26;  // Ready for DMA command
    GPUSTAT |= (readMode == ReadMode::Vram) << 27;
    GPUSTAT |= 1 << 28;  // Ready for receive DMA block
    GPUSTAT |= (dmaDirection & 3) << 29;
    GPUSTAT |= odd << 31;

    return GPUSTAT;
}

uint32_t GPU::read(uint32_t address) {
    int reg = address & 0xfffffffc;
    if (reg == 0) {
        if (readMode == ReadMode::Vram) {
            return readVramData();
        } else {
            return readData;
        }
    }
    if (reg == 4) {
        uint32_t stat = getStat();

        if (verbose) {
            fmt::print("[GPU] R GPUSTAT: 0x{:07x}\n", stat);
        }
        return stat;
    }
    return 0;
}

void GPU::write(uint32_t address, uint32_t data) {
    if (address == 0) writeGP0(data);
    if (address == 4) writeGP1(data);
}

void GPU::writeGP0(uint32_t data) {
    if (cmd == Command::None) {
        command = data >> 24;
        arguments[0] = data;
        argumentCount = 0;
        currentArgument = 1;

        if (command == 0x00) {
            // NOP
            if (verbose && arguments[0] != 0x000000) {
                fmt::print("[GPU] GP0(0) nop: non-zero argument (0x{:06x})\n", arguments[0]);
            }
        } else if (command == 0x01) {
            // Clear Cache
            clutCachePos = ivec2(-1, -1);
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
        } else if (command >= 0x80 && command <= 0x9f) {
            // Copy rectangle (VRAM -> VRAM)
            cmd = Command::CopyVramToVram;
            argumentCount = 3;
        } else if (command >= 0xa0 && command <= 0xbf) {
            // Copy rectangle (CPU -> VRAM)
            cmd = Command::CopyCpuToVram1;
            argumentCount = 2;
        } else if (command >= 0xc0 && command <= 0xdf) {
            // Copy rectangle (VRAM -> CPU)
            cmd = Command::CopyVramToCpu;
            argumentCount = 2;
        } else if (command == 0xe1) {
            // Draw mode setting
            gp0_e1._reg = arguments[0];
            if (!textureDisableAllowed) {
                gp0_e1.textureDisable = false;
            }
        } else if (command == 0xe2) {
            // Texture window setting
            gp0_e2._reg = arguments[0];
        } else if (command == 0xe3) {
            // Drawing area top left
            drawingArea.left = arguments[0] & 0x3ff;
            drawingArea.top = (arguments[0] & 0xffc00) >> 10;
        } else if (command == 0xe4) {
            // Drawing area bottom right
            drawingArea.right = arguments[0] & 0x3ff;
            drawingArea.bottom = (arguments[0] & 0xffc00) >> 10;
        } else if (command == 0xe5) {
            // Drawing offset
            drawingOffsetX = extend_sign<11>(arguments[0] & 0x7ff);
            drawingOffsetY = extend_sign<11>((arguments[0] >> 11) & 0x7ff);
        } else if (command == 0xe6) {
            // Mask bit setting
            gp0_e6._reg = arguments[0];
        } else if (command == 0x1f) {
            // Interrupt request
            irqRequest = true;
            sys->interrupt->trigger(interrupt::IrqNumber::GPU);
        } else {
            fmt::print("GPU: GP0(0x{:02x}) args 0x{:06x}\n", command, arguments[0]);
        }

        if (gpuLogEnabled && cmd == Command::None) {
            gpuLogList.push_back(LogEntry::GP0(arguments[0]));
        }
        // TODO: Refactor gpu log to handle copies && multiline

        argumentCount++;
        return;
    }

    if (currentArgument < argumentCount) {
        // Multi line draw call is handled separately
        // as it line count is not known beforehand
        if (cmd == Command::Line && LineArgs(command).polyLine && ((data & 0xf000f000) == 0x50005000)) {
            cmd = Command::None;
            return;
        }

        arguments[currentArgument++] = data;
        if (currentArgument != argumentCount) {
            return;
        }
    }

    if (gpuLogEnabled) {
        if (cmd == Command::CopyCpuToVram2) {
            // Find last gp0(0xa0) command
            int n = 5;
            for (int i = gpuLogList.size() - 1; i >= 0; i--) {
                auto& last = gpuLogList[i];
                if (last.cmd() == 0xa0) {
                    last.args.push_back(arguments[0]);
                    break;
                }
                if (n-- == 0) break;
            }
        } else {
            LogEntry entry;
            entry.type = 0;
            entry.args = std::vector<uint32_t>(arguments.begin(), arguments.begin() + argumentCount);
            gpuLogList.push_back(entry);
        }
    }
    if (verbose && cmd != Command::CopyCpuToVram2) {
        fmt::print("[GPU] W GP0(0x{:02x}): 0x{:06x}\n", command, arguments[0]);
    }

    if (cmd == Command::FillRectangle) {
        cmdFillRectangle();
    } else if (cmd == Command::Polygon) {
        cmdPolygon(command);
    } else if (cmd == Command::Line) {
        cmdLine(command);
    } else if (cmd == Command::Rectangle) {
        cmdRectangle(command);
    } else if (cmd == Command::CopyCpuToVram1) {
        cmdCpuToVram1();
    } else if (cmd == Command::CopyCpuToVram2) {
        cmdCpuToVram2();
    } else if (cmd == Command::CopyVramToCpu) {
        cmdVramToCpu();
    } else if (cmd == Command::CopyVramToVram) {
        cmdVramToVram();
    }
}

void GPU::writeGP1(uint32_t data) {
    uint8_t command = (data >> 24) & 0x3f;
    uint32_t argument = data & 0xffffff;

    if (command == 0x00) {  // Reset GPU
        reset();
    } else if (command == 0x01) {  // Reset command buffer
        cmd = Command::None;
    } else if (command == 0x02) {  // Acknowledge IRQ1
        irqRequest = false;
    } else if (command == 0x03) {  // Display Enable
        displayDisable = argument & 1;
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
        displayRangeY2 = argument >> 10;
    } else if (command == 0x08) {  // Display mode
        gp1_08._reg = argument;
    } else if (command == 0x09) {  // Allow texture disable
        textureDisableAllowed = argument & 1;
    } else if (command >= 0x10 && command <= 0x1f) {  // get GPU Info
        readMode = ReadMode::Register;
        argument &= 0xf;

        if (argument == 2) {
            readData = gp0_e2._reg;
        } else if (argument == 3) {
            readData = (drawingArea.top << 10) | drawingArea.left;
        } else if (argument == 4) {
            readData = (drawingArea.bottom << 10) | drawingArea.right;
        } else if (argument == 5) {
            readData = ((drawingOffsetY & 0x7ff) << 11) | (drawingOffsetX & 0x7ff);
        } else if (argument == 7) {
            readData = 2;  // GPU Version
        } else if (argument == 8) {
            readData = 0;
        } else {
            // readData unchanged
        }
    } else if (command == 0x20) {
        fmt::print("[GPU] GP1(0x20) - Special Texture disable: 0x{:06x}\n", argument);
    } else {
        fmt::print("[GPU] GP1(0x{:02x}) args 0x{:06x}\n", command, argument);
        assert(false);
    }
    if (verbose) {
        fmt::print("[GPU] W GP1(0x{:02x}): 0x{:06x}\n", command, argument);
    }
    if (gpuLogEnabled) {
        gpuLogList.push_back(LogEntry::GP1(data));
    }
}

float GPU::cyclesPerLine() const {
    if (isNtsc())
        return timing::CYCLES_PER_LINE_NTSC;
    else
        return timing::CYCLES_PER_LINE_PAL;
}

int GPU::linesPerFrame() const {
    if (isNtsc())
        return timing::LINES_TOTAL_NTSC;
    else
        return timing::LINES_TOTAL_PAL;
}

bool GPU::emulateGpuCycles(int cycles) {
    gpuDot += cycles;

    int newLines = gpuDot / 3413;
    if (newLines == 0) return false;
    gpuDot %= 3413;
    gpuLine += newLines;

    if (gpuLine < linesPerFrame() - 20 - 1) {
        if (gp1_08.verticalResolution == GP1_08::VerticalResolution::r480 && gp1_08.interlace) {
            odd = (frames % 2) != 0;
        } else {
            odd = (gpuLine % 2) != 0;
        }
    } else {
        odd = false;
    }

    if (gpuLine == linesPerFrame() - 1) {
        gpuLine = 0;
        frames++;
        return true;
    }
    return false;
}

int GPU::minDrawingX(int x) const { return std::max((int)drawingArea.left, std::max(0, x)); }

int GPU::minDrawingY(int y) const { return std::max((int)drawingArea.top, std::max(0, y)); }

int GPU::maxDrawingX(int x) const { return std::min((int)drawingArea.right, std::min(VRAM_WIDTH, x)); }

int GPU::maxDrawingY(int y) const { return std::min((int)drawingArea.bottom, std::min(VRAM_HEIGHT, y)); }

bool GPU::insideDrawingArea(int x, int y) const {
    return (x >= drawingArea.left) && (x < drawingArea.right) && (x < VRAM_WIDTH) && (y >= drawingArea.top) && (y < drawingArea.bottom)
           && (y < VRAM_HEIGHT);
}

bool GPU::isNtsc() const { return forceNtsc || gp1_08.videoMode == GP1_08::VideoMode::ntsc; }

void GPU::dumpVram() {
    const char* dumpName = "vram.png";
    std::vector<uint8_t> vram(VRAM_WIDTH * VRAM_HEIGHT * 3);

    for (size_t i = 0; i < this->vram.size(); i++) {
        PSXColor c(this->vram[i]);
        vram[i * 3 + 0] = c.r << 3;
        vram[i * 3 + 1] = c.g << 3;
        vram[i * 3 + 2] = c.b << 3;
    }

    stbi_write_png(dumpName, VRAM_WIDTH, VRAM_HEIGHT, 3, vram.data(), VRAM_WIDTH * 3);
}
}  // namespace gpu