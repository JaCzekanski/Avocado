#include <imgui.h>
#include <magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include "../../../cpu/gte/gte.h"
#include "debugger/debugger.h"
#include "gui.h"
#include "imgui/imgui_memory_editor.h"
#include "renderer/opengl/shader/texture.h"
#include "tools.h"
#include "utils/file.h"
#include "utils/string.h"

extern bool showVramWindow;
extern bool showWatchWindow;

struct Watch {
    uint32_t address;
    int size;  // 1, 2, 4
    std::string name;
};
std::vector<Watch> watches = {{0x1F801070, 2, "ISTAT"}, {0x1F801074, 2, "IMASK"}};

struct Area {
    std::string name;
    ImVec2 pos;
    ImVec2 size;
};

float blinkTimer = 0.f;
std::vector<Area> vramAreas;

const char *mapIo(uint32_t address) {
    address -= 0x1f801000;

#define IO(begin, end, periph)                   \
    if (address >= (begin) && address < (end)) { \
        return periph;                           \
    }

    IO(0x00, 0x24, "memoryControl");
    IO(0x40, 0x50, "controller");
    IO(0x50, 0x60, "serial");
    IO(0x60, 0x64, "memoryControl");
    IO(0x70, 0x78, "interrupt");
    IO(0x80, 0x100, "dma");
    IO(0x100, 0x110, "timer0");
    IO(0x110, 0x120, "timer1");
    IO(0x120, 0x130, "timer2");
    IO(0x800, 0x804, "cdrom");
    IO(0x810, 0x818, "gpu");
    IO(0x820, 0x828, "mdec");
    IO(0xC00, 0x1000, "spu");
    IO(0x1000, 0x1043, "exp2");
    return "";
}

void replayCommands(gpu::GPU *gpu, int to) {
    using gpu::Command;

    auto commands = gpu->gpuLogList;
    gpu->vram = gpu->prevVram;

    gpu->gpuLogEnabled = false;
    for (int i = 0; i <= to; i++) {
        auto cmd = commands.at(i);

        if (cmd.args.size() == 0) printf("Panic! no args");

        for (size_t j = 0; j < cmd.args.size(); j++) {
            uint32_t arg = cmd.args[j];

            if (j == 0) arg |= cmd.command << 24;

            if (j == 0 && i == to && (cmd.cmd == Command::Polygon || cmd.cmd == Command::Rectangle || cmd.cmd == Command::Line)) {
                arg = (cmd.command << 24) | 0x00ff00;
                if (cmd.cmd == Command::Polygon || cmd.cmd == Command::Rectangle) arg &= ~(1 << 24);
            }

            gpu->write(0, arg);
        }
    }
    gpu->gpuLogEnabled = true;
}

void dumpRegister(const char *name, uint16_t reg) {
    ImGui::BulletText("%s", name);
    ImGui::NextColumn();
    ImGui::Text("0x%04x", reg);
    ImGui::NextColumn();
}

void gteRegistersWindow(GTE &gte) {
    if (!gteRegistersEnabled) {
        return;
    }
    ImGui::Begin("GTE registers", &gteRegistersEnabled, ImVec2(400, 400));

    ImGui::Columns(3, nullptr, false);
    ImGui::Text("IR1:  %04hX", gte.ir[1]);
    ImGui::NextColumn();
    ImGui::Text("IR2:  %04hX", gte.ir[2]);
    ImGui::NextColumn();
    ImGui::Text("IR3:  %04hX", gte.ir[3]);
    ImGui::NextColumn();

    ImGui::Separator();
    ImGui::Text("MAC0: %08X", gte.mac[0]);

    ImGui::Separator();
    ImGui::Text("MAC1: %08X", gte.mac[1]);
    ImGui::NextColumn();
    ImGui::Text("MAC2: %08X", gte.mac[2]);
    ImGui::NextColumn();
    ImGui::Text("MAC3: %08X", gte.mac[3]);
    ImGui::NextColumn();

    ImGui::Separator();
    ImGui::Text("TRX:  %08X", gte.translation.x);
    ImGui::NextColumn();
    ImGui::Text("TRY:  %08X", gte.translation.y);
    ImGui::NextColumn();
    ImGui::Text("TRZ:  %08X", gte.translation.z);
    ImGui::NextColumn();

    ImGui::Separator();

    for (int i = 0; i < 4; i++) {
        ImGui::Text("S%dX:  %04hX", i, gte.s[i].x);
        ImGui::NextColumn();
        ImGui::Text("S%dY:  %04hX", i, gte.s[i].y);
        ImGui::NextColumn();
        ImGui::Text("S%dZ:  %04hX", i, gte.s[i].z);
        ImGui::NextColumn();
    }

    ImGui::Separator();

    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            ImGui::Text("RT%d%d:  %04hX", y + 1, x + 1, gte.rotation[y][x]);
            ImGui::NextColumn();
        }
    }

    ImGui::Separator();
    ImGui::Text("VX0:  %04hX", gte.v[0].x);
    ImGui::NextColumn();
    ImGui::Text("VY0:  %04hX", gte.v[0].y);
    ImGui::NextColumn();
    ImGui::Text("VZ0:  %04hX", gte.v[0].z);
    ImGui::NextColumn();

    ImGui::Separator();
    ImGui::Text("OFX:  %08X", gte.of[0]);
    ImGui::NextColumn();
    ImGui::Text("OFY:  %08X", gte.of[1]);
    ImGui::NextColumn();
    ImGui::Text("H:   %04hX", gte.h);
    ImGui::NextColumn();

    ImGui::Separator();
    ImGui::Text("DQA:  %04hX", gte.dqa);
    ImGui::NextColumn();
    ImGui::Text("DQB:  %08X", gte.dqb);
    ImGui::NextColumn();

    ImGui::End();
}

void gteLogWindow(System *sys) {
    if (!gteLogEnabled) {
        return;
    }
    static char filterBuffer[16];
    static bool searchActive = false;
    bool filterActive = strlen(filterBuffer) > 0;
    ImGui::Begin("GTE Log", &gteLogEnabled, ImVec2(300, 400));

    ImGui::BeginChild("GTE Log", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    for (size_t i = 0; i < sys->cpu->gte.log.size(); i++) {
        auto ioEntry = sys->cpu->gte.log.at(i);
        std::string t;
        if (ioEntry.mode == GTE::GTE_ENTRY::MODE::func) {
            t = string_format("%5d %c 0x%02x", i, 'F', ioEntry.n);
        } else {
            t = string_format("%5d %c %2d: 0x%08x", i, ioEntry.mode == GTE::GTE_ENTRY::MODE::read ? 'R' : 'W', ioEntry.n, ioEntry.data);
        }
        if (filterActive && t.find(filterBuffer) != std::string::npos)  // if found
        {
            // if search enabled - continue
            ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "%s", t.c_str());
            continue;
        }
        if (searchActive) continue;

        ImGui::Text("%s", t.c_str());
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();

    ImGui::Text("Filter");
    ImGui::SameLine();
    if (ImGui::InputText("", filterBuffer, sizeof(filterBuffer) / sizeof(char), ImGuiInputTextFlags_EnterReturnsTrue)) {
        searchActive = !searchActive;
    }

    ImGui::End();
}

void gpuLogWindow(System *sys) {
    vramAreas.clear();
    if (!gpuLogEnabled) {
        return;
    }
    static std::unique_ptr<Texture> textureImage;
    static std::vector<uint8_t> textureUnpacked;
    if (!textureImage) {
        textureImage = std::make_unique<Texture>(512, 512, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, false);
        textureUnpacked.resize(512 * 512 * 4);
    }

    ImGui::Begin("GPU Log", &gpuLogEnabled, ImVec2(300, 400));

    ImGui::BeginChild("GPU Log", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    // TODO: Find last values
    gpu::GP0_E1 last_e1 = sys->gpu->gp0_e1;
    int16_t last_offset_x = sys->gpu->drawingOffsetX;
    int16_t last_offset_y = sys->gpu->drawingOffsetY;

    int renderTo = -1;
    ImGuiListClipper clipper((int)sys->gpu.get()->gpuLogList.size());
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            auto &entry = sys->gpu.get()->gpuLogList[i];
            bool nodeOpen = ImGui::TreeNode((void *)(intptr_t)i, "%4d cmd: 0x%02x  %s", i, entry.command,
                                            std::string(magic_enum::enum_name(entry.cmd)).c_str());
            bool isHovered = ImGui::IsItemHovered();

            if (isHovered) {
                renderTo = i;
            }

            // Analyze
            if (isHovered || nodeOpen) {
                bool showArguments = true;
                float color[3];
                color[0] = (entry.args[0] & 0xff) / 255.f;
                color[1] = ((entry.args[0] >> 8) & 0xff) / 255.f;
                color[2] = ((entry.args[0] >> 16) & 0xff) / 255.f;

                uint8_t command = entry.command;
                auto arguments = entry.args;
                if (command >= 0x20 && command < 0x40) {
                    auto arg = gpu::PolygonArgs(command);
                    (void)arg;  // TODO: Parse Polygon commands
                } else if (command >= 0x40 && command < 0x60) {
                    auto arg = gpu::LineArgs(command);
                    (void)arg;  // TODO: Parse Line commands
                } else if (command >= 0x60 && command < 0x80) {
                    auto arg = gpu::RectangleArgs(command);
                    int16_t w = arg.getSize();
                    int16_t h = arg.getSize();

                    if (arg.size == 0) {
                        w = extend_sign<10>(arguments[(arg.isTextureMapped ? 3 : 2)] & 0xffff);
                        h = extend_sign<10>((arguments[(arg.isTextureMapped ? 3 : 2)] & 0xffff0000) >> 16);
                    }

                    int16_t x = extend_sign<10>(arguments[1] & 0xffff);
                    int16_t y = extend_sign<10>((arguments[1] & 0xffff0000) >> 16);

                    x += last_offset_x;
                    y += last_offset_y;

                    if (nodeOpen) {
                        std::string flags;
                        if (arg.semiTransparency) flags += "semi-transparent, ";
                        if (arg.isTextureMapped) flags += "textured, ";
                        if (!arg.isRawTexture) flags += "color-blended, ";
                        ImGui::Text("Flags: %s", flags.c_str());
                        ImGui::Text("Pos: %dx%d", x, y);
                        ImGui::Text("size: %dx%d", w, h);

                        ImGui::NewLine();
                        ImGui::Text("Color: ");
                        ImGui::SameLine();
                        ImGui::ColorEdit3("##color", color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoInputs);
                    }

                    vramAreas.push_back({string_format("Rectangle %d", i), ImVec2(x, y), ImVec2(w, h)});

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

                        texX += last_e1.texturePageBaseX * 64;
                        texY += last_e1.texturePageBaseY * 256;

                        std::string textureInfo = string_format("Texture (%d bit)", textureBits);

                        if (nodeOpen) {
                            ImGui::NewLine();
                            ImGui::Text("%s: ", textureInfo.c_str());
                            ImGui::Text("Pos:  %dx%d", texX, texY);
                            ImGui::Text("CLUT: %dx%d", clutX, clutY);
                            // ImGui::SameLine();
                            // ImGui::Image()
                            // TODO: Render texture
                        }

                        vramAreas.push_back({textureInfo, ImVec2(texX, texY), ImVec2(textureWidth, h)});

                        if (clutColors != 0) {
                            vramAreas.push_back({"CLUT", ImVec2(clutX, clutY), ImVec2(clutColors, 1)});
                        }
                    }

                    showArguments = false;
                }

                if (nodeOpen) {
                    if (showArguments) {
                        // Render arguments
                        for (auto &arg : entry.args) {
                            ImGui::Text("- 0x%08x", arg);
                        }
                    }
                    ImGui::TreePop();
                }
            }
        }
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();

    if (ImGui::Button("Dump")) {
        ImGui::OpenPopup("Save dump dialog");
    }

    if (ImGui::BeginPopupModal("Save dump dialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        char filename[32];
        ImGui::Text("File: ");
        ImGui::SameLine();

        ImGui::PushItemWidth(140);
        if (ImGui::InputText("", filename, 31, ImGuiInputTextFlags_EnterReturnsTrue)) {
            auto gpu = sys->gpu.get();
            auto gpuLog = gpu->gpuLogList;
            nlohmann::json j;

            for (size_t i = 0; i < gpuLog.size(); i++) {
                auto e = gpuLog[i];
                j.push_back({{"command", e.command}, {"cmd", (int)e.cmd}, {"name", magic_enum::enum_name(e.cmd)}, {"args", e.args}});
            }

            putFileContents(string_format("%s.json", filename), j.dump(2));

            // Binary vram dump
            std::vector<uint8_t> vram;
            for (auto d : gpu->vram) {
                vram.push_back(d & 0xff);
                vram.push_back((d >> 8) & 0xff);
            }
            putFileContents(string_format("%s.bin", filename), vram);

            ImGui::CloseCurrentPopup();
        }
        ImGui::PopItemWidth();

        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::Text("(press Enter to add)");
        ImGui::EndPopup();
    }

    ImGui::End();

    if (sys->state != System::State::run && renderTo >= 0) {
        replayCommands(sys->gpu.get(), renderTo);
    }
}

void ioLogWindow(System *sys) {
#ifdef ENABLE_IO_LOG
    if (!ioLogEnabled) {
        return;
    }
    ImGui::Begin("IO Log", &ioLogEnabled, ImVec2(200, 400));

    ImGui::BeginChild("IO Log", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImGuiListClipper clipper((int)sys->ioLogList.size());
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            auto ioEntry = sys->ioLogList.at(i);
            char mode = ioEntry.mode == System::IO_LOG_ENTRY::MODE::READ ? 'R' : 'W';
            ImGui::Text("%c %2d 0x%08x: 0x%0*x %*s %s                  (pc: 0x%08x)", mode, ioEntry.size, ioEntry.addr, ioEntry.size / 4,
                        ioEntry.data,
                        // padding
                        8 - ioEntry.size / 4, "", mapIo(ioEntry.addr), ioEntry.pc);
        }
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();

    ImGui::End();
#endif
}

void vramWindow(gpu::GPU *gpu) {
    static std::unique_ptr<Texture> vramImage;
    static std::vector<uint8_t> vramUnpacked;
    if (!vramImage) {
        vramImage = std::make_unique<Texture>(gpu::VRAM_WIDTH, gpu::VRAM_HEIGHT, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, false);
        vramUnpacked.resize(gpu::VRAM_WIDTH * gpu::VRAM_HEIGHT * 4);
    }

    // Update texture
    for (int y = 0; y < gpu::VRAM_HEIGHT; y++) {
        for (int x = 0; x < gpu::VRAM_WIDTH; x++) {
            PSXColor c = gpu->vram[y * gpu::VRAM_WIDTH + x];

            vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 4 + 0] = c.r << 3;
            vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 4 + 1] = c.g << 3;
            vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 4 + 2] = c.b << 3;
            vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 4 + 3] = 0xff;
        }
    }
    vramImage->update(vramUnpacked.data());

    blinkTimer += 0.0025 * M_PI;
    ImColor blinkColor = ImColor::HSV(blinkTimer, 1.f, 1.f, 0.75f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0, 0.0, 0.0, 1.0));

    auto defaultSize = ImVec2(1024, 512 + ImGui::GetItemsLineHeightWithSpacing() * 2);
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(defaultSize.x / 2, defaultSize.y / 2), ImVec2(defaultSize.x * 2, defaultSize.y * 2),
        [](ImGuiSizeCallbackData *data) { data->DesiredSize.y = (data->DesiredSize.x / 2) + ImGui::GetItemsLineHeightWithSpacing() * 2; });

    ImGui::Begin("VRAM", &showVramWindow, defaultSize, -1, ImGuiWindowFlags_NoScrollbar);

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

void watchWindow(System *sys) {
    static int selectedWatch = -1;
    ImGui::Begin("Watch", &showWatchWindow, ImVec2(300, 200));

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));
    ImGui::BeginChild("Watch", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), true);
    int i = 0;
    for (auto &watch : watches) {
        uint32_t value = 0;
        if (watch.size == 1)
            value = sys->readMemory8(watch.address);
        else if (watch.size == 2)
            value = sys->readMemory16(watch.address);
        else if (watch.size == 4)
            value = sys->readMemory32(watch.address);
        else
            continue;

        ImVec4 color = ImVec4(1.f, 1.f, 1.f, 1.f);
        ImGui::PushStyleColor(ImGuiCol_Text, color);

        ImGui::Selectable(string_format("0x%08x: 0x%0*x    %s", watch.address, watch.size * 2, value, watch.name.c_str()).c_str());

        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGui::GetIO().MouseClicked[1])) {
            ImGui::OpenPopup("watch_menu");
            selectedWatch = i;
        }
        i++;
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();

    bool showPopup = false;
    if (ImGui::BeginPopupContextItem("watch_menu")) {
        if (selectedWatch != -1 && ImGui::Selectable("Remove")) {
            watches.erase(watches.begin() + selectedWatch);
            selectedWatch = -1;
        }
        if (ImGui::Selectable("Add")) showPopup = true;

        ImGui::EndPopup();
    }
    if (showPopup) ImGui::OpenPopup("Add watch");

    if (ImGui::BeginPopupModal("Add watch", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static const char *sizeLabels[] = {"1 byte (8 bits)", "2 bytes (16 bits)", "4 bytes (32 bits)"};
        static int selectedSize = 0;
        static char name[64];
        static char addressInput[10];
        uint32_t address;

        ImGui::Text("Size: ");
        ImGui::SameLine();
        ImGui::PushItemWidth(160);
        ImGui::Combo("", &selectedSize, sizeLabels, 3);
        ImGui::PopItemWidth();

        ImGui::Text("Address: ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::InputText("##address", addressInput, 9, ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::PopItemWidth();

        ImGui::Text("Name: ");
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::InputText("##name", name, 64);
        ImGui::PopItemWidth();

        if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
        ImGui::SameLine();
        if (ImGui::Button("Add")) {
            if (sscanf(addressInput, "%x", &address) == 1) {
                int size = 1;
                if (selectedSize == 0)
                    size = 1;
                else if (selectedSize == 1)
                    size = 2;
                else if (selectedSize == 2)
                    size = 4;

                watches.push_back({address, size, name});
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    ImGui::Text("Use right mouse button to show menu");
    ImGui::End();
}

void ramWindow(System *sys) {
    static MemoryEditor editor;
    editor.DrawWindow("Ram", sys->ram, System::RAM_SIZE);
}