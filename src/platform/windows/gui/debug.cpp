#include <imgui.h>
#include <nlohmann/json.hpp>
#include <vector>
#include "../../../cpu/gte/gte.h"
#include "debugger/debugger.h"
#include "gui.h"
#include "imgui/imgui_memory_editor.h"
#include "utils/string.h"

extern bool showVramWindow;
extern bool showDisassemblyWindow;
extern bool showBreakpointsWindow;
extern bool showWatchWindow;

struct Watch {
    uint32_t address;
    int size;  // 1, 2, 4
    std::string name;
};
std::vector<Watch> watches = {{0x1F801070, 2, "ISTAT"}, {0x1F801074, 2, "IMASK"}};

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
    ImGui::Text("TRX:  %08X", gte.tr.x);
    ImGui::NextColumn();
    ImGui::Text("TRY:  %08X", gte.tr.y);
    ImGui::NextColumn();
    ImGui::Text("TRZ:  %08X", gte.tr.z);
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

    ImGui::Text("RT11:  %04hX", gte.rt.v11);
    ImGui::NextColumn();
    ImGui::Text("RT12:  %04hX", gte.rt.v12);
    ImGui::NextColumn();
    ImGui::Text("RT13:  %04hX", gte.rt.v13);
    ImGui::NextColumn();

    ImGui::Text("RT21:  %04hX", gte.rt.v21);
    ImGui::NextColumn();
    ImGui::Text("RT22:  %04hX", gte.rt.v22);
    ImGui::NextColumn();
    ImGui::Text("RT23:  %04hX", gte.rt.v23);
    ImGui::NextColumn();

    ImGui::Text("RT31:  %04hX", gte.rt.v31);
    ImGui::NextColumn();
    ImGui::Text("RT32:  %04hX", gte.rt.v32);
    ImGui::NextColumn();
    ImGui::Text("RT33:  %04hX", gte.rt.v33);
    ImGui::NextColumn();

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
    if (!gpuLogEnabled) {
        return;
    }
    ImGui::Begin("GPU Log", &gpuLogEnabled, ImVec2(300, 400));

    ImGui::BeginChild("GPU Log", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    int renderTo = -1;
    ImGuiListClipper clipper(sys->gpu.get()->gpuLogList.size());
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            auto &entry = sys->gpu.get()->gpuLogList[i];

            bool nodeOpen = ImGui::TreeNode((void *)(intptr_t)i, "cmd: 0x%02x  %s", entry.command, gpu::CommandStr[(int)entry.cmd]);

            if (ImGui::IsItemHovered()) {
                renderTo = i;
            }

            if (nodeOpen) {
                // Render arguments
                for (auto &arg : entry.args) {
                    ImGui::Text("- 0x%08x", arg);
                }
                ImGui::TreePop();
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
                j.push_back({{"command", e.command}, {"cmd", (int)e.cmd}, {"name", gpu::CommandStr[(int)e.cmd]}, {"args", e.args}});
            }

            putFileContents(string_format("%s.json", filename), j.dump(2));

            // Binary vram dump
            std::vector<uint8_t> vram;
            vram.assign(gpu->vram.begin(), gpu->vram.end());
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

void ioWindow(System *sys) {
    if (!showIo) {
        return;
    }
    ImGui::Begin("IO", &showIo, ImVec2(300, 200));

    ImGui::Columns(1, nullptr, false);
    ImGui::Text("Timer 0");

    ImGui::Columns(2, nullptr, false);
    dumpRegister("current", sys->timer0->current._reg);
    dumpRegister("target", sys->timer0->target._reg);
    dumpRegister("mode", sys->timer0->mode._reg);

    ImGui::Columns(1, nullptr, false);
    ImGui::Text("Timer 1");

    ImGui::Columns(2, nullptr, false);
    dumpRegister("current", sys->timer1->current._reg);
    dumpRegister("target", sys->timer1->target._reg);
    dumpRegister("mode", sys->timer1->mode._reg);

    ImGui::Columns(1, nullptr, false);
    ImGui::Text("Timer 2");

    ImGui::Columns(2, nullptr, false);
    dumpRegister("current", sys->timer2->current._reg);
    dumpRegister("target", sys->timer2->target._reg);
    dumpRegister("mode", sys->timer2->mode._reg);

    ImGui::End();
}

void ioLogWindow(System *sys) {
#ifdef ENABLE_IO_LOG
    if (!ioLogEnabled) {
        return;
    }
    ImGui::Begin("IO Log", &ioLogEnabled, ImVec2(200, 400));

    ImGui::BeginChild("IO Log", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImGuiListClipper clipper(sys->ioLogList.size());
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

void vramWindow() {
    auto defaultSize = ImVec2(1024, 512 + 32);
    ImGui::Begin("VRAM", &showVramWindow, defaultSize, -1, ImGuiWindowFlags_NoScrollbar);
    auto currentSize = ImGui::GetWindowSize();
    currentSize.y -= 32;
    ImGui::Image((ImTextureID)(uintptr_t)vramTextureId, currentSize);
    ImGui::End();
}

std::string formatOpcode(mips::Opcode &opcode) {
    auto disasm = debugger::decodeInstruction(opcode);
    return string_format("%s %*s %s", disasm.mnemonic.c_str(), 6 - disasm.mnemonic.length(), "", disasm.parameters.c_str());
}

void disassemblyWindow(System *sys) {
    static uint32_t startAddress = 0;
    static bool lockAddress = false;

    ImGui::Begin("Disassembly", &showDisassemblyWindow, ImVec2(400, 500));

    if (ImGui::Button(sys->state == System::State::run ? "Pause" : "Run")) {
        if (sys->state == System::State::run)
            sys->state = System::State::pause;
        else
            sys->state = System::State::run;
    }
    ImGui::SameLine();
    if (ImGui::Button("Step")) {
        sys->singleStep();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Map register names", &debugger::mapRegisterNames);

    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));
    ImGui::Columns(4);
    for (int i = 0; i < 34; i++) {
        if (i == 0)
            ImGui::Text("PC: 0x%08x", sys->cpu->PC);
        else if (i == 32)
            ImGui::Text("hi: 0x%08x", sys->cpu->hi);
        else if (i == 33)
            ImGui::Text("lo: 0x%08x", sys->cpu->lo);
        else
            ImGui::Text("%s: 0x%08x", debugger::reg(i).c_str(), sys->cpu->reg[i]);

        ImGui::NextColumn();
    }
    ImGui::Columns(1);

    ImGui::NewLine();

    ImGui::BeginChild("Disassembly", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), true);
    if (!lockAddress) startAddress = sys->cpu->PC;
    for (int i = -15; i < 15; i++) {
        uint32_t address = (startAddress + i * 4) & 0xFFFFFFFC;
        mips::Opcode opcode(sys->readMemory32(address));

        ImVec4 color = ImVec4(1.f, 1.f, 1.f, 1.f);
        if (i == 0)
            color = ImVec4(1.f, 1.f, 0.f, 1.f);
        else if (sys->cpu->breakpoints.find(address) != sys->cpu->breakpoints.end())
            color = ImVec4(1.f, 0.f, 0.f, 1.f);
        ImGui::PushStyleColor(ImGuiCol_Text, color);

        if (ImGui::Selectable(string_format("0x%08x: %s", address, formatOpcode(opcode).c_str()).c_str())) {
            auto bp = sys->cpu->breakpoints.find(address);
            if (bp == sys->cpu->breakpoints.end()) {
                sys->cpu->breakpoints.emplace(address, mips::CPU::Breakpoint());
            } else {
                sys->cpu->breakpoints.erase(bp);
            }
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%02x %02x %02x %02x", opcode.opcode & 0xff, (opcode.opcode >> 8) & 0xff, (opcode.opcode >> 16) & 0xff,
                              (opcode.opcode >> 24) & 0xff);
        }

        ImGui::PopStyleColor();
    }
    ImGui::EndChild();

    ImGui::PopStyleVar();

    ImGui::Text("Go to address ");
    ImGui::SameLine();

    ImGui::PushItemWidth(80.f);
    char addressInput[10];
    if (ImGui::InputText("", addressInput, 9, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (sscanf(addressInput, "%x", &startAddress) == 1)
            lockAddress = true;
        else if (strlen(addressInput) == 0)
            lockAddress = false;
    }
    ImGui::PopItemWidth();

    ImGui::End();
}

void breakpointsWindow(System *sys) {
    static uint32_t selectedBreakpoint = 0;
    ImGui::Begin("Breakpoints", &showBreakpointsWindow, ImVec2(300, 200));

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));
    ImGui::BeginChild("Breakpoints", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), true);
    for (auto &bp : sys->cpu->breakpoints) {
        mips::Opcode opcode(sys->readMemory32(bp.first));

        ImVec4 color = ImVec4(1.f, 1.f, 1.f, 1.f);
        if (!bp.second.enabled) color = ImVec4(0.5f, 0.5f, 0.5f, 1.f);
        ImGui::PushStyleColor(ImGuiCol_Text, color);

        if (ImGui::Selectable(
                string_format("0x%08x: %s (hit count: %d)", bp.first, formatOpcode(opcode).c_str(), bp.second.hitCount).c_str())) {
            bp.second.enabled = !bp.second.enabled;
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGui::GetIO().MouseClicked[1])) {
            ImGui::OpenPopup("breakpoint_menu");
            selectedBreakpoint = bp.first;
        }

        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();

    bool showPopup = false;
    if (ImGui::BeginPopupContextItem("breakpoint_menu")) {
        auto breakpointExist = sys->cpu->breakpoints.find(selectedBreakpoint) != sys->cpu->breakpoints.end();

        if (breakpointExist && ImGui::Selectable("Remove")) sys->cpu->breakpoints.erase(selectedBreakpoint);
        if (ImGui::Selectable("Add")) showPopup = true;

        ImGui::EndPopup();
    }
    if (showPopup) ImGui::OpenPopup("Add breakpoint");

    if (ImGui::BeginPopupModal("Add breakpoint", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        char addressInput[10];
        uint32_t address;
        ImGui::Text("Address: ");
        ImGui::SameLine();

        ImGui::PushItemWidth(80);
        if (ImGui::InputText("", addressInput, 9, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)
            && sscanf(addressInput, "%x", &address) == 1) {
            sys->cpu->breakpoints.emplace(address, mips::CPU::Breakpoint());
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

    ImGui::Text("Use right mouse button to show menu");
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