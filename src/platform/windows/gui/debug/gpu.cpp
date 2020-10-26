#include "gpu.h"
#define _USE_MATH_DEFINES
#include <fmt/core.h>
#include <imgui.h>
#include <math.h>
#include <magic_enum.hpp>
#include "config.h"
#include "platform/windows/gui/images.h"
#include "renderer/opengl/opengl.h"
#include "system.h"
#include "utils/file.h"

namespace gui::debug {
GPU::GPU() {
    busToken = bus.listen<Event::Config::Graphics>([&](auto) {
        textureImage.release();
        vramImage.release();
    });
}

GPU::~GPU() { bus.unlistenAll(busToken); }

void GPU::registersWindow(System *sys) {
    auto &gpu = sys->gpu;

    ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
    ImGui::Begin("GPU", &registersWindowOpen);

    int horRes = gpu->gp1_08.getHorizontalResoulution();
    int verRes = gpu->gp1_08.getVerticalResoulution();
    bool interlaced = gpu->gp1_08.interlace;
    int mode = gpu->gp1_08.videoMode == gpu::GP1_08::VideoMode::ntsc ? 60 : 50;
    int colorDepth = gpu->gp1_08.colorDepth == gpu::GP1_08::ColorDepth::bit24 ? 24 : 15;

    ImGui::Text("Display:");
    ImGui::Text("Resolution %d:%d%c @ %dHz", horRes, verRes, interlaced ? 'i' : 'p', mode);
    ImGui::Text("Color depth: %dbit", colorDepth);
    ImGui::Text("areaStart:  %4d:%4d", gpu->displayAreaStartX, gpu->displayAreaStartY);
    ImGui::Text("rangeX:     %4d:%4d", gpu->displayRangeX1, gpu->displayRangeX2);
    ImGui::Text("rangeY:     %4d:%4d", gpu->displayRangeY1, gpu->displayRangeY2);

    ImGui::Text("");
    ImGui::Text("Drawing:");
    ImGui::Text("areaMin:    %4d:%4d", gpu->drawingArea.left, gpu->drawingArea.top);
    ImGui::Text("areaMax:    %4d:%4d", gpu->drawingArea.right, gpu->drawingArea.bottom);
    ImGui::Text("offset:     %4d:%4d", gpu->drawingOffsetX, gpu->drawingOffsetY);
    // ImGui::Text("")

    ImGui::End();
}

void replayCommands(gpu::GPU *gpu, int to) {
    using gpu::Command;

    auto commands = gpu->gpuLogList;
    gpu->vram = gpu->prevVram;

    gpu->gpuLogEnabled = false;
    for (int i = 0; i <= to; i++) {
        auto cmd = commands.at(i);

        if (cmd.args.size() == 0) fmt::print("Panic! no args");

        for (uint32_t arg : cmd.args) {
            uint8_t addr = (cmd.type == 0) ? 0 : 4;
            gpu->write(addr, arg);
        }
    }
    gpu->gpuLogEnabled = true;
}

// Helpers
namespace {
std::string getGP0CommandName(gpu::LogEntry &entry) {
    switch (entry.cmd()) {
        case 0x00: return "nop";
        case 0x01: return "clear cache";
        case 0x02: return "fill rectangle";
        case 0x1f: return "requestIrq";
        case 0x20 ... 0x3f: return "polygon";
        case 0x40 ... 0x5f: return "line";
        case 0x60 ... 0x7f: return "rectangle";
        case 0x80 ... 0x9f: return "copy vram -> vram";
        case 0xa0 ... 0xbf: return "copy cpu -> vram";
        case 0xc0 ... 0xdf: return "copy vram -> cpu";
        case 0xe1: return "set drawMode";
        case 0xe2: return "set textureWindow";
        case 0xe3: {
            uint16_t left = entry.args[0] & 0x3ff;
            uint16_t top = (entry.args[0] & 0xffc00) >> 10;

            return fmt::format("set drawAreaBegin [{}:{}]", left, top);
        }
        case 0xe4: {
            uint16_t right = entry.args[0] & 0x3ff;
            uint16_t bottom = (entry.args[0] & 0xffc00) >> 10;
            return fmt::format("set drawAreaEnd   [{}:{}]", right, bottom);
        }
        case 0xe5: {
            int16_t drawingOffsetX = extend_sign<11>(entry.args[0] & 0x7ff);
            int16_t drawingOffsetY = extend_sign<11>((entry.args[0] >> 11) & 0x7ff);
            return fmt::format("set drawOffset    [{}:{}]", drawingOffsetX, drawingOffsetY);
        }
        case 0xe6: return "set maskBit";
        default: return "UNKNOWN";
    }
};

std::string getGP1CommandName(gpu::LogEntry &entry) {
    switch (entry.cmd()) {
        case 0x00: return "Reset GPU";
        case 0x01: return "Reset command buffer";
        case 0x02: return "Acknowledge IRQ1";
        case 0x03: return "Display Enable";
        case 0x04: {
            return fmt::format("DMA Direction    [{}]", entry.args[0] & 3);
        }
        case 0x05: return "Start of display area";
        case 0x06: return "Horizontal display range";
        case 0x07: return "Vertical display range";
        case 0x08: return "Display mode";
        case 0x09: return "Allow texture disable";
        case 0x10 ... 0x1f: return "Get GPU info";
        case 0x20: return "Special texture disable";
        default: return "UNKNOWN";
    }
};

bool isCommandImportant(uint8_t cmd) {
    if (cmd == 0x02 || (cmd >= 0x20 && cmd < 0xe0)) {
        return true;
    } else {
        return false;
    }
}
};  // namespace

void GPU::handlePolygonCommand(gpu::PolygonArgs arg, const std::vector<uint32_t> &arguments) {
    int ptr = 1;

    primitive::Triangle::Vertex v[4];
    gpu::TextureInfo tex;

    for (int i = 0; i < arg.getVertexCount(); i++) {
        v[i].pos.x = extend_sign<11>(arguments[ptr] & 0xffff);
        v[i].pos.y = extend_sign<11>((arguments[ptr++] & 0xffff0000) >> 16);

        if (!arg.isRawTexture && (!arg.gouraudShading || i == 0)) v[i].color.raw = arguments[0] & 0xffffff;
        if (arg.isTextureMapped) {
            if (i == 0) tex.palette = arguments[ptr];
            if (i == 1) tex.texpage = arguments[ptr];
            v[i].uv.x = arguments[ptr] & 0xff;
            v[i].uv.y = (arguments[ptr] >> 8) & 0xff;
            ptr++;
        }
        if (arg.gouraudShading && i < arg.getVertexCount() - 1) v[i + 1].color.raw = arguments[ptr++] & 0xffffff;
    }

    std::string flags;
    if (arg.semiTransparency) flags += "semi-transparent, ";  // TODO: print WHICH transparency is used, magic enum
    if (arg.isTextureMapped) flags += "textured, ";           // TODO: Bits?
    if (!arg.isRawTexture) flags += "color-blended, ";
    if (arg.gouraudShading) flags += "gouraud-shaded";
    ImGui::Text("Flags: %s", flags.c_str());
    for (int i = 0; i < arg.getVertexCount(); i++) {
        auto text = fmt::format("v{}: {}x{}", i, v[i].pos.x + last_offset_x, v[i].pos.y + last_offset_y);
        if (arg.isTextureMapped) {
            text += fmt::format(", uv{}: {}x{}", i, v[i].uv.x, v[i].uv.y);
        }
        ImGui::TextUnformatted(text.c_str());
    }

    ImGui::NewLine();
    ImGui::Text("Color: ");
    ImGui::SameLine();
    ImGui::ColorEdit3("##color", color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoInputs);

    // vramAreas.push_back({fmt::format("Rectangle {}", i), ImVec2(x, y), ImVec2(w, h)});

    if (arg.isTextureMapped) {
        // int clutColors = tex.getBitcount();
        // int textureWidth;
        // int textureBits;

        // if (last_e1.texturePageColors == gpu::GP0_E1::TexturePageColors::bit4) {
        //     clutColors = 16;
        //     textureWidth = w / 4;
        //     textureBits = 4;
        // } else if (last_e1.texturePageColors == gpu::GP0_E1::TexturePageColors::bit8) {
        //     clutColors = 256;
        //     textureWidth = w / 2;
        //     textureBits = 8;
        // } else {
        //     clutColors = 0;
        //     textureWidth = w;
        //     textureBits = 16;
        // }

        std::string textureInfo = fmt::format("Texture ({} bit)", tex.getBitcount());

        ImGui::NewLine();
        ImGui::Text("%s: ", textureInfo.c_str());
        ImGui::Text("texPage: %d:%d", tex.getBaseX(), tex.getBaseY());
        ImGui::Text("CLUT:    %d:%d", tex.getClutX(), tex.getClutY());

        // vramAreas.push_back({textureInfo, ImVec2(texX, texY), ImVec2(textureWidth, h)});

        if (tex.getBitcount() != 0) {
            vramAreas.push_back({"CLUT", ImVec2(tex.getClutX(), tex.getClutY()), ImVec2(tex.getBitcount(), 1)});
        }
    }
}

void GPU::handleLineCommand(gpu::LineArgs arg, const std::vector<uint32_t> &arguments) {
    (void)arg;  // TODO: Parse Line commands
}

void GPU::handleRectangleCommand(gpu::RectangleArgs arg, const std::vector<uint32_t> &arguments) {
    int16_t w = arg.getSize();
    int16_t h = arg.getSize();

    if (arg.size == 0) {
        w = extend_sign<11>(arguments[(arg.isTextureMapped ? 3 : 2)] & 0xffff);
        h = extend_sign<11>((arguments[(arg.isTextureMapped ? 3 : 2)] & 0xffff0000) >> 16);
    }

    int16_t x = extend_sign<11>(arguments[1] & 0xffff);
    int16_t y = extend_sign<11>((arguments[1] & 0xffff0000) >> 16);

    x += last_offset_x;
    y += last_offset_y;

    std::string flags;
    if (arg.semiTransparency) flags += "semi-transparent, ";
    if (arg.isTextureMapped) flags += "textured, ";
    if (!arg.isRawTexture) flags += "color-blended, ";
    ImGui::Text("Flags: %s", flags.c_str());
    ImGui::Text("Pos: %d:%d", x, y);
    ImGui::Text("size: %d:%d", w, h);

    ImGui::NewLine();
    ImGui::Text("Color: ");
    ImGui::SameLine();
    ImGui::ColorEdit3("##color", color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoInputs);

    vramAreas.push_back({fmt::format("Rectangle {}", /*i*/ -1), ImVec2(x, y), ImVec2(w, h)});

    if (arg.isTextureMapped) {
        int texX = arguments[2] & 0xff;
        int texY = (arguments[2] & 0xff00) >> 8;
        int clutX = ((arguments[2] >> 16) & 0x3f) * 16;
        int clutY = ((arguments[2] >> 22) & 0x1ff);
        int clutColors;
        int textureWidth;
        int textureBits;

        if (last_e1.texturePageColors == gpu::GP0_E1::TexturePageColors::bit4) {
            clutColors = 16;
            textureWidth = w / 4;
            textureBits = 4;
        } else if (last_e1.texturePageColors == gpu::GP0_E1::TexturePageColors::bit8) {
            clutColors = 256;
            textureWidth = w / 2;
            textureBits = 8;
        } else {
            clutColors = 0;
            textureWidth = w;
            textureBits = 16;
        }

        int texPageX = last_e1.texturePageBaseX * 64;
        int texPageY = last_e1.texturePageBaseY * 256;

        std::string textureInfo = fmt::format("Texture ({} bit)", textureBits);

        ImGui::NewLine();
        ImGui::Text("%s: ", textureInfo.c_str());
        ImGui::Text("UV:      %d:%d", texX + texPageX, texY + texPageY);
        ImGui::Text("CLUT:    %d:%d", clutX, clutY);
        ImGui::NewLine();
        ImGui::Text("Pos:     %d:%d (raw value in draw call)", texX, texY);
        ImGui::Text("texPage: %d:%d (from latest GP0_E1)", texPageX, texPageY);
        // ImGui::SameLine();
        // ImGui::Image()
        // TODO: Render texture

        vramAreas.push_back({textureInfo, ImVec2(texPageX + texX / (16 / textureBits), texPageY + texY), ImVec2(textureWidth, h)});

        if (clutColors != 0) {
            vramAreas.push_back({"CLUT", ImVec2(clutX, clutY), ImVec2(clutColors, 1)});
        }
    }
}

void GPU::printCommandDetails(gpu::LogEntry &entry) {
    uint8_t command = entry.cmd();
    color[0] = (entry.args[0] & 0xff) / 255.f;
    color[1] = ((entry.args[0] >> 8) & 0xff) / 255.f;
    color[2] = ((entry.args[0] >> 16) & 0xff) / 255.f;

    if (command >= 0x20 && command < 0x40) {
        handlePolygonCommand(command, entry.args);
    } else if (command >= 0x40 && command < 0x60) {
        handleLineCommand(command, entry.args);
    } else if (command >= 0x60 && command < 0x80) {
        handleRectangleCommand(command, entry.args);
    } else if (command >= 0x80 && command <= 0x9f) {
        uint16_t srcX = entry.args[1] & 0xffff;
        uint16_t srcY = (entry.args[1] >> 16) & 0xffff;

        uint16_t dstX = entry.args[2] & 0xffff;
        uint16_t dstY = (entry.args[2] >> 16) & 0xffff;

        uint16_t width = entry.args[3] & 0xffff;
        uint16_t height = (entry.args[3] >> 16) & 0xffff;

        ImGui::Text("src:  %d:%d", srcX, srcY);
        ImGui::Text("dst:  %d:%d", dstX, dstY);
        ImGui::Text("size: %d:%d", width, height);
    } else if (command >= 0xa0 && command <= 0xdf) {
        uint16_t srcX = entry.args[1] & 0xffff;
        uint16_t srcY = (entry.args[1] >> 16) & 0xffff;

        uint16_t width = entry.args[2] & 0xffff;
        uint16_t height = (entry.args[2] >> 16) & 0xffff;

        ImGui::Text("dst:  %d:%d", srcX, srcY);
        ImGui::Text("size: %d:%d", width, height);
    } else if (command == 0xe1) {
        gpu::GP0_E1 e1;
        e1._reg = entry.args[0];

#define ENUM_NAME(x) (std::string(magic_enum::enum_name(x)).c_str())
        ImGui::Text("Texture page base:       %d:%d", e1.texturePageBaseX * 64, e1.texturePageBaseY * 256);
        ImGui::Text("Semi transparency:       %s", ENUM_NAME(e1.semiTransparency));
        ImGui::Text("Texture bit depth:       %s", ENUM_NAME(e1.texturePageColors));
        ImGui::Text("Dither 24bit:            %d", e1.dither24to15);
        ImGui::Text("Drawing to display area: %s", ENUM_NAME(e1.drawingToDisplayArea));

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.f));
        ImGui::Text("Texture disable:         %d", e1.textureDisable);
        ImGui::Text("Rect texture X flip:     %d", e1.texturedRectangleXFlip);
        ImGui::Text("Rect texture Y flip:     %d", e1.texturedRectangleYFlip);
        ImGui::PopStyleColor();

        last_e1 = e1;
    } else if (command == 0xe2) {
        gpu::GP0_E2 e2;
        e2._reg = entry.args[0];

        ImGui::Text("mask:   %d:%d", e2.maskX, e2.maskY);
        ImGui::Text("offset: %d:%d", e2.offsetX, e2.offsetY);
    } else if (command == 0xe3) {
        uint16_t left = entry.args[0] & 0x3ff;
        uint16_t top = (entry.args[0] & 0xffc00) >> 10;

        ImGui::Text("x0 y0:   %d:%d", left, top);
    } else if (command == 0xe4) {
        uint16_t right = entry.args[0] & 0x3ff;
        uint16_t bottom = (entry.args[0] & 0xffc00) >> 10;

        ImGui::Text("x1 y1:   %d:%d", right, bottom);
    } else if (command == 0xe5) {
        int16_t drawingOffsetX = extend_sign<11>(entry.args[0] & 0x7ff);
        int16_t drawingOffsetY = extend_sign<11>((entry.args[0] >> 11) & 0x7ff);

        ImGui::Text("x y:   %d:%d", drawingOffsetX, drawingOffsetY);

        last_offset_x = drawingOffsetX;
        last_offset_y = drawingOffsetY;
    } else if (command == 0xe6) {
        gpu::GP0_E6 e6;
        e6._reg = entry.args[0];

        ImGui::Text("setMaskWhileDrawing: %d", e6.setMaskWhileDrawing);
        ImGui::Text("checkMaskBeforeDraw: %d", e6.checkMaskBeforeDraw);
    }
}

void GPU::logWindow(System *sys) {
    vramAreas.clear();
    if (!textureImage) {
        textureImage = std::make_unique<Texture>(512, 512, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, false);
        textureUnpacked.resize(512 * 512 * 4);
    }

    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("GPU Log", &logWindowOpen);

    ImGui::BeginChild("GPU Log", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    last_e1 = sys->gpu->gp0_e1;
    last_offset_x = sys->gpu->drawingOffsetX;
    last_offset_y = sys->gpu->drawingOffsetY;

    int renderTo = -1;
    ImGuiListClipper clipper((int)sys->gpu.get()->gpuLogList.size());
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            auto &entry = sys->gpu.get()->gpuLogList[i];

            ImVec4 entryColor = ImVec4(1.f, 1.f, 1.f, 1.f);
            if (entry.type == 1) {  // GP1
                entryColor = ImVec4(0.4f, 0.4f, 0.8f, 1.f);
            } else if (!isCommandImportant(entry.cmd())) {
                entryColor = ImVec4(0.4f, 0.4f, 0.4f, 1.f);
            }

            std::string cmdDescription = "";
            if (entry.type == 0) {
                cmdDescription = getGP0CommandName(entry);
            } else if (entry.type == 1) {
                cmdDescription = getGP1CommandName(entry);
            }

            ImGui::PushStyleColor(ImGuiCol_Text, entryColor);
            bool nodeOpen = ImGui::TreeNode((void *)(intptr_t)i,
                                            fmt::format("{:4d} GP{}(0x{:02x}) {}", i, entry.type, entry.cmd(), cmdDescription).c_str());
            ImGui::PopStyleColor();

            bool isHovered = ImGui::IsItemHovered();

            if (isHovered) {
                renderTo = i;
            }

            if (nodeOpen) {
                printCommandDetails(entry);
                ImGui::TreePop();
            } else if (isHovered) {
                ImGui::BeginTooltip();
                printCommandDetails(entry);
                ImGui::EndPopup();
            }
        }
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::End();

    if (sys->state != System::State::run && renderTo >= 0) {
        replayCommands(sys->gpu.get(), renderTo);
    }
}

void GPU::vramWindow(gpu::GPU *gpu) {
    if (!vramImage) {
        vramImage = std::make_unique<Texture>(gpu::VRAM_WIDTH, gpu::VRAM_HEIGHT, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, false);
        vramUnpacked.resize(gpu::VRAM_WIDTH * gpu::VRAM_HEIGHT * 3);
    }

    // Update texture
    for (int y = 0; y < gpu::VRAM_HEIGHT; y++) {
        for (int x = 0; x < gpu::VRAM_WIDTH; x++) {
            PSXColor c = gpu->vram[y * gpu::VRAM_WIDTH + x];

            vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 3 + 0] = c.r << 3;
            vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 3 + 1] = c.g << 3;
            vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 3 + 2] = c.b << 3;
        }
    }
    vramImage->update(vramUnpacked.data());

    blinkTimer += 0.0025 * M_PI;
    ImColor blinkColor = ImColor::HSV(blinkTimer, 1.f, 1.f, 0.75f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0, 0.0, 0.0, 1.0));

    auto defaultSize = ImVec2(1024, 512 + ImGui::GetFrameHeightWithSpacing() * 2);
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(defaultSize.x / 2, defaultSize.y / 2), ImVec2(defaultSize.x * 2, defaultSize.y * 2),
        [](ImGuiSizeCallbackData *data) { data->DesiredSize.y = (data->DesiredSize.x / 2) + ImGui::GetFrameHeightWithSpacing() * 2; });

    ImGui::SetNextWindowSize(defaultSize);
    ImGui::Begin("VRAM", &vramWindowOpen, ImGuiWindowFlags_NoScrollbar);

    auto currentSize = ImGui::GetWindowContentRegionMax();
    currentSize.y = currentSize.x / 2;

    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImGui::Image((ImTextureID)(uintptr_t)vramImage->get(), currentSize);

    if (ImGui::Button("Original size")) {
        ImGui::SetWindowSize(defaultSize);
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    float scale = currentSize.x / defaultSize.x;

    ImDrawList *drawList = ImGui::GetWindowDrawList();
    for (auto area : vramAreas) {
        ImVec2 a, b;
        a.x = cursorPos.x + area.pos.x * scale;
        a.y = cursorPos.y + area.pos.y * scale;

        b.x = cursorPos.x + (area.pos.x + area.size.x) * scale;
        b.y = cursorPos.y + (area.pos.y + area.size.y) * scale;

        drawList->AddRectFilled(a, b, blinkColor, 0.f, 0);

        if (ImGui::IsMouseHoveringRect(a, b)) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(area.name.c_str());
            ImGui::EndTooltip();
        }
    }

    ImGui::End();
}

void GPU::displayWindows(System *sys) {
    if (registersWindowOpen) registersWindow(sys);
    if (logWindowOpen) logWindow(sys);
    if (vramWindowOpen) vramWindow(sys->gpu.get());
}
};  // namespace gui::debug