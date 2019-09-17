#include <fmt/core.h>
#include <imgui.h>
#include <magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include "../../../cpu/gte/gte.h"
#include "config.h"
#include "debugger/debugger.h"
#include "gui.h"
#include "imgui/imgui_memory_editor.h"
#include "renderer/opengl/shader/texture.h"
#include "tools.h"
#include "utils/file.h"

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

        if (cmd.args.size() == 0) fmt::print("Panic! no args");

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

        ImGui::Selectable(fmt::format("0x{:08x}: 0x{:0{}x}    {}", watch.address, value, watch.size * 2, watch.name).c_str());

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
    editor.DrawWindow("Ram", sys->ram.data(), System::RAM_SIZE);
}