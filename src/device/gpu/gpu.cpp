#include "gpu.h"
#include <fmt/core.h>
#include <cassert>
#include "config.h"
#include "render/render.h"
#include "utils/file.h"
#include "utils/logic.h"
#include "utils/macros.h"

namespace gpu {
GPU::GPU() {
    busToken = bus.listen<Event::Config::Graphics>([&](auto) { reload(); });
    reload();
    reset();
}

GPU::~GPU() { bus.unlistenAll(busToken); }

void GPU::reload() {
    forceNtsc = config["options"]["graphics"]["forceNtsc"];
    auto mode = config["options"]["graphics"]["rendering_mode"].get<RenderingMode>();
    softwareRendering = (mode & RenderingMode::SOFTWARE) != 0;
    hardwareRendering = (mode & RenderingMode::HARDWARE) != 0;
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
}

void GPU::drawTriangle(const primitive::Triangle& triangle) {
    if (hardwareRendering) {
        int flags = 0;
        if (triangle.isSemiTransparent) flags |= Vertex::Flags::SemiTransparency;
        if (triangle.isRawTexture) flags |= Vertex::Flags::RawTexture;
        if (gp0_e1.dither24to15) flags |= Vertex::Flags::Dithering;
        if (triangle.gouroudShading) flags |= Vertex::Flags::GouroudShading;
        flags |= static_cast<int>(triangle.transparency) << 5;

        for (int i : {0, 1, 2}) {
            auto& v = triangle.v[i];
            vertices.push_back({Vertex::Type::Polygon,
                                {v.pos.x + drawingOffsetX, v.pos.y + drawingOffsetY},
                                {v.color.r, v.color.g, v.color.b},
                                {v.uv.x, v.uv.y},
                                triangle.bits,
                                {triangle.clut.x, triangle.clut.y},
                                {triangle.texpage.x, triangle.texpage.y},
                                flags,
                                gp0_e2,
                                gp0_e6});
        }
    }

    if (softwareRendering) {
        Render::drawTriangle(this, triangle);
    }
}

void GPU::drawLine(const primitive::Line& line) {
    if (hardwareRendering) {
        glm::ivec2 p[2];
        for (int i = 0; i < 2; i++) {
            p[i] = line.pos[i];
            p[i].x += drawingOffsetX;
            p[i].y += drawingOffsetY;
        }
        auto c = line.color;
        int flags = 0;

        if (line.isSemiTransparent) flags |= Vertex::Flags::SemiTransparency;
        if (line.gouroudShading) flags |= Vertex::Flags::GouroudShading;

        vertices.push_back(
            {Vertex::Type::Line, {p[0].x, p[0].y}, {c[0].r, c[0].g, c[0].b}, {0, 0}, 0, {0, 0}, {0, 0}, flags, gp0_e2, gp0_e6});
        vertices.push_back(
            {Vertex::Type::Line, {p[1].x, p[1].y}, {c[1].r, c[1].g, c[1].b}, {0, 0}, 0, {0, 0}, {0, 0}, flags, gp0_e2, gp0_e6});
    }

    if (softwareRendering) {
        Render::drawLine(this, line);
    }
}

void GPU::drawRectangle(const primitive::Rect& rect) {
    if (hardwareRendering) {
        glm::ivec2 p;
        int x[4], y[4];
        glm::ivec2 uv[4];

        p.x = rect.pos.x + drawingOffsetX;
        p.y = rect.pos.y + drawingOffsetY;

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
        if (rect.isSemiTransparent) flags |= Vertex::Flags::SemiTransparency;

        Vertex v[6];
        for (int i : {0, 1, 2, 1, 2, 3}) {
            v[i] = {Vertex::Type::Polygon,
                    {x[i], y[i]},
                    {rect.color.r, rect.color.g, rect.color.b},
                    {uv[i].x, uv[i].y},
                    rect.bits,
                    {rect.clut.x, rect.clut.y},
                    {rect.texpage.x, rect.texpage.y},
                    flags,
                    gp0_e2,
                    gp0_e6};
            vertices.push_back(v[i]);
        }
    }

    if (softwareRendering) {
        Render::drawRectangle(this, rect);
    }
}

void GPU::cmdFillRectangle(uint8_t command) {
    (void)command;

    // I'm sorry, but it appears that C++ doesn't have local functions.
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
        glm::ivec2 p[4] = {{startX, startY}, {endX, startY}, {startX, endY}, {endX, endY}};

        RGB c;
        c.raw = arguments[0];

        GP0_E2 e2;
        GP0_E6 e6;

        Vertex v[6];
        for (int i : {0, 1, 2, 1, 2, 3}) {
            v[i] = {Vertex::Type::Polygon,
                    {p[i].x, p[i].y},
                    {c.r, c.g, c.b},
                    {0, 0},  // UV: 0
                    0,       // Bits: 0
                    {0, 0},  // clut: 0
                    {0, 0},  // texPage: 0
                    0,       // Flags: 0
                    e2,      // gp0_e2: 0
                    e6};     // gp0_e6: 0, no mask
            vertices.push_back(v[i]);
        }
    }
}

void GPU::cmdPolygon(PolygonArgs arg) {
    int ptr = 1;

    primitive::Triangle::Vertex v[4];
    TextureInfo tex;

    for (int i = 0; i < arg.getVertexCount(); i++) {
        v[i].pos.x = extend_sign<10>(arguments[ptr] & 0xffff);
        v[i].pos.y = extend_sign<10>((arguments[ptr++] & 0xffff0000) >> 16);

        if (!arg.isRawTexture && (!arg.gouroudShading || i == 0)) v[i].color.raw = arguments[0] & 0xffffff;
        if (arg.isTextureMapped) {
            if (i == 0) tex.palette = arguments[ptr];
            if (i == 1) tex.texpage = arguments[ptr];
            v[i].uv.x = arguments[ptr] & 0xff;
            v[i].uv.y = (arguments[ptr] >> 8) & 0xff;
            ptr++;
        }
        if (arg.gouroudShading && i < arg.getVertexCount() - 1) v[i + 1].color.raw = arguments[ptr++] & 0xffffff;
    }

    primitive::Triangle triangle;

    for (int i : {0, 1, 2}) triangle.v[i] = v[i];

    triangle.bits = 0;
    triangle.isSemiTransparent = arg.semiTransparency;
    triangle.transparency = gp0_e1.semiTransparency;
    triangle.isRawTexture = arg.isRawTexture;
    triangle.gouroudShading = arg.gouroudShading;

    if (arg.isTextureMapped) {
        triangle.bits = tex.getBitcount();
        triangle.texpage.x = tex.getBaseX();
        triangle.texpage.y = tex.getBaseY();
        triangle.clut.x = tex.getClutX();
        triangle.clut.y = tex.getClutY();
        triangle.transparency = tex.semiTransparencyBlending();
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

// fixme: handle multiline with > 15 lines (arguments array hold only 31 elements)
void GPU::cmdLine(LineArgs arg) {
    int ptr = 1;
    int lineCount = (!arg.polyLine) ? 1 : 15;

    primitive::Line line;
    line.isSemiTransparent = arg.semiTransparency;
    line.gouroudShading = arg.gouroudShading;

    for (int i = 0; i < lineCount; i++) {
        if ((arguments[ptr] & 0xf000f000) == 0x50005000) break;

        if (i == 0) {
            line.pos[0].x = extend_sign<10>(arguments[ptr] & 0xffff);
            line.pos[0].y = extend_sign<10>((arguments[ptr++] & 0xffff0000) >> 16);
            line.color[0].raw = (arguments[0] & 0xffffff);
        } else {
            line.pos[0] = line.pos[1];
            line.pos[0] = line.pos[1];
            line.color[0] = line.color[1];
        }

        if (arg.gouroudShading) {
            line.color[1].raw = arguments[ptr++];
        } else {
            line.color[1].raw = (arguments[0] & 0xffffff);
        }

        line.pos[1].x = extend_sign<10>((arguments[ptr] & 0xffff));
        line.pos[1].y = extend_sign<10>((arguments[ptr++] & 0xffff0000) >> 16);

        drawLine(line);
    }

    cmd = Command::None;
}

void GPU::cmdRectangle(RectangleArgs arg) {
    /* Rectangle command format
    arg[0] = cmd + color      (0xCCBB GGRR)
    arg[1] = pos              (0xYYYY XXXX)
    arg[2] = Palette + tex UV (0xCLUT VVUU) (*if textured)
    arg[3] = size             (0xHHHH WWWW) (*if variable size) */
    using vec2 = glm::ivec2;
    using Bits = gpu::GP0_E1::TexturePageColors;

    int16_t w = arg.getSize();
    int16_t h = arg.getSize();

    if (arg.size == 0) {
        w = extend_sign<10>(arguments[(arg.isTextureMapped ? 3 : 2)] & 0xffff);
        h = extend_sign<10>((arguments[(arg.isTextureMapped ? 3 : 2)] & 0xffff0000) >> 16);
    }

    int16_t x = extend_sign<10>(arguments[1] & 0xffff);
    int16_t y = extend_sign<10>((arguments[1] & 0xffff0000) >> 16);

    primitive::Rect rect;
    rect.pos = vec2(x, y);
    rect.size = vec2(w, h);
    rect.color.raw = (arguments[0] & 0xffffff);

    if (!arg.isTextureMapped) {
        rect.bits = 0;
    } else if (gp0_e1.texturePageColors == Bits::bit4) {
        rect.bits = 4;
    } else if (gp0_e1.texturePageColors == Bits::bit8) {
        rect.bits = 8;
    } else if (gp0_e1.texturePageColors == Bits::bit15) {
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

        rect.uv = vec2(p.u, p.v);
        rect.clut = vec2(p.clutX * 16, p.clutY);
        rect.texpage = vec2(gp0_e1.texturePageBaseX * 64, gp0_e1.texturePageBaseY * 256);
    }

    drawRectangle(rect);

    cmd = Command::None;
}

struct MaskCopy {
    constexpr static int startX(int x) { return x & 0x3ff; }
    constexpr static int startY(int y) { return y & 0x1ff; }
    constexpr static int endX(int x) { return ((x - 1) & 0x3ff) + 1; }
    constexpr static int endY(int y) { return ((y - 1) & 0x1ff) + 1; }
};

void GPU::cmdCpuToVram1(uint8_t command) {
    (void)command;

    if ((arguments[0] & 0x00ffffff) != 0) {
        fmt::print("[GPU] cmdCpuToVram1: Suspicious arg0: 0x{:x}\n", arguments[0]);
    }
    startX = currX = MaskCopy::startX(arguments[1] & 0xffff);
    startY = currY = MaskCopy::startY((arguments[1] & 0xffff0000) >> 16);

    endX = startX + MaskCopy::endX(arguments[2] & 0xffff);
    endY = startY + MaskCopy::endY((arguments[2] & 0xffff0000) >> 16);

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

void GPU::cmdCpuToVram2(uint8_t command) {
    (void)command;

    uint32_t byte = arguments[0];

    // TODO: ugly code
    maskedWrite(currX++, currY, byte & 0xffff);
    if (currX >= endX) {
        currX = startX;
        if (++currY >= endY) cmd = Command::None;
    }

    maskedWrite(currX++, currY, (byte >> 16) & 0xffff);
    if (currX >= endX) {
        currX = startX;
        if (++currY >= endY) cmd = Command::None;
    }

    currentArgument = 0;
}

void GPU::cmdVramToCpu(uint8_t command) {
    (void)command;

    if ((arguments[0] & 0x00ffffff) != 0) {
        fmt::print("[GPU] cmdVramToCpu: Suspicious arg0: 0x{:x}\n", arguments[0]);
    }
    gpuReadMode = 1;
    startX = currX = MaskCopy::startX(arguments[1] & 0xffff);
    startY = currY = MaskCopy::startY((arguments[1] & 0xffff0000) >> 16);
    endX = startX + MaskCopy::endX(arguments[2] & 0xffff);
    endY = startY + MaskCopy::endY((arguments[2] & 0xffff0000) >> 16);

    cmd = Command::None;
}

void GPU::cmdVramToVram(uint8_t command) {
    (void)command;

    if ((arguments[0] & 0x00ffffff) != 0) {
        fmt::print("[GPU] cpuVramToVram: Suspicious arg0: 0x{:x}, breaking!!!\n", arguments[0]);
        cmd = Command::None;
        return;
    }
    int srcX = MaskCopy::startX(arguments[1] & 0xffff);
    int srcY = MaskCopy::startY((arguments[1] & 0xffff0000) >> 16);

    int dstX = MaskCopy::startX(arguments[2] & 0xffff);
    int dstY = MaskCopy::startY((arguments[2] & 0xffff0000) >> 16);

    int width = MaskCopy::endX(arguments[3] & 0xffff);
    int height = MaskCopy::endY((arguments[3] & 0xffff0000) >> 16);

    if (width > VRAM_WIDTH || height > VRAM_HEIGHT) {
        fmt::print("[GPU] cpuVramToVram: Suspicious width: 0x{:x} or height: 0x{:x}\n", width, height);
        cmd = Command::None;
        return;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint16_t src = VRAM[(srcY + y) % VRAM_HEIGHT][(srcX + x) % VRAM_WIDTH];
            maskedWrite(dstX + x, dstY + y, src);
        }
    }

    cmd = Command::None;
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
    GPUSTAT |= 1 << 26;  // Ready for DMA command
    GPUSTAT |= (cmd != Command::CopyCpuToVram2) << 27;
    GPUSTAT |= 1 << 28;  // Ready for receive DMA block
    GPUSTAT |= (dmaDirection & 3) << 29;
    GPUSTAT |= odd << 31;
}

uint32_t GPU::read(uint32_t address) {
    int reg = address & 0xfffffffc;
    if (reg == 0) {
        if (gpuReadMode == 0 || gpuReadMode == 2) {
            return GPUREAD;
        }
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
            if (arguments[0] != 0x000000) {
                fmt::print("[GPU] GP0(0) nop: non-zero argument (0x{:06x})\n", arguments[0]);
            }
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
            drawingArea.left = arguments[0] & 0x3ff;
            drawingArea.top = (arguments[0] & 0xffc00) >> 10;
        } else if (command == 0xe4) {
            // Drawing area bottom right
            drawingArea.right = arguments[0] & 0x3ff;
            drawingArea.bottom = (arguments[0] & 0xffc00) >> 10;
        } else if (command == 0xe5) {
            // Drawing offset
            drawingOffsetX = extend_sign<10>(arguments[0] & 0x7ff);
            drawingOffsetY = extend_sign<10>((arguments[0] >> 11) & 0x7ff);
        } else if (command == 0xe6) {
            // Mask bit setting
            gp0_e6._reg = arguments[0];
        } else if (command == 0x1f) {
            // Interrupt request
            irqRequest = true;
            // TODO: IRQ
        } else {
            fmt::print("GPU: GP0(0x{:02x}) args 0x{:06x}\n", command, arguments[0]);
        }

        if (gpuLogEnabled && cmd == Command::None) {
            LogEntry entry;
            entry.cmd = Command::Extra;
            entry.command = command;
            entry.args = std::vector<uint32_t>();
            entry.args.push_back(arguments[0]);
            gpuLogList.push_back(entry);
        }

        argumentCount++;
        return;
    }

    if (currentArgument < argumentCount) {
        arguments[currentArgument++] = data;
        if (argumentCount == MAX_ARGS && (data & 0xf000f000) == 0x50005000) argumentCount = currentArgument;
        if (currentArgument != argumentCount) return;
    }

    if (gpuLogEnabled && cmd != Command::CopyCpuToVram2) {
        LogEntry entry;
        entry.cmd = cmd;
        entry.command = command;
        entry.args = std::vector<uint32_t>(arguments.begin(), arguments.begin() + argumentCount);
        gpuLogList.push_back(entry);
    }

    if (cmd == Command::FillRectangle)
        cmdFillRectangle(command);
    else if (cmd == Command::Polygon)
        cmdPolygon(command);
    else if (cmd == Command::Line)
        cmdLine(command);
    else if (cmd == Command::Rectangle)
        cmdRectangle(command);
    else if (cmd == Command::CopyCpuToVram1)
        cmdCpuToVram1(command);
    else if (cmd == Command::CopyCpuToVram2)
        cmdCpuToVram2(command);
    else if (cmd == Command::CopyVramToCpu)
        cmdVramToCpu(command);
    else if (cmd == Command::CopyVramToVram)
        cmdVramToVram(command);
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
        displayRangeY2 = argument >> 10;
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
            GPUREAD = (drawingArea.top << 10) | drawingArea.left;
        } else if (argument == 4) {
            GPUREAD = (drawingArea.bottom << 10) | drawingArea.right;
        } else if (argument == 5) {
            GPUREAD = ((drawingOffsetY & 0x7ff) << 11) | (drawingOffsetX & 0x7ff);
        } else if (argument == 7) {
            GPUREAD = 2;  // GPU Version
        } else if (argument == 8) {
            GPUREAD = 0;
        } else {
            // GPUREAD unchanged
        }
    } else {
        fmt::print("[GPU] GP1(0x{:02x}) args 0x{:06x}\n", command, argument);
        assert(false);
    }
    // command 0x20 is not implemented
}

bool GPU::emulateGpuCycles(int cycles) {
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

int GPU::minDrawingX(int x) const { return std::max((int)drawingArea.left, std::max(0, x)); }

int GPU::minDrawingY(int y) const { return std::max((int)drawingArea.top, std::max(0, y)); }

int GPU::maxDrawingX(int x) const { return std::min((int)drawingArea.right, std::min(VRAM_WIDTH, x)); }

int GPU::maxDrawingY(int y) const { return std::min((int)drawingArea.bottom, std::min(VRAM_HEIGHT, y)); }

bool GPU::insideDrawingArea(int x, int y) const {
    return (x >= drawingArea.left) && (x < drawingArea.right) && (x < VRAM_WIDTH) && (y >= drawingArea.top) && (y < drawingArea.bottom)
           && (y < VRAM_HEIGHT);
}

bool GPU::isNtsc() { return forceNtsc || gp1_08.videoMode == GP1_08::VideoMode::ntsc; }

void GPU::dumpVram() {
    std::vector<uint8_t> vram;
    vram.assign(this->vram.begin(), this->vram.end());
    putFileContents("vram.bin", vram);
}
}  // namespace gpu