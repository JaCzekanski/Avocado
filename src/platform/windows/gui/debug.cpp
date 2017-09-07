#include "gui.h"
#include <imgui.h>
#include "imgui/imgui_memory_editor.h"
#include "utils/string.h"
#include "gte.h"
#include "debugger/debugger.h"

extern bool showVramWindow;
extern bool showDisassemblyWindow;
extern bool showBreakpointsWindow;

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

void replayCommands(mips::gpu::GPU *gpu, int to) {
    using namespace device::gpu;
    auto commands = gpu->gpuLogList;
    gpu->vram = gpu->prevVram;

    gpu->gpuLogEnabled = false;
    for (int i = 0; i <= to; i++) {
        auto cmd = commands.at(i);

        if (cmd.args.size() == 0) printf("Panic! no args");

        for (int j = 0; j < cmd.args.size(); j++) {
            uint32_t arg = cmd.args[j];

            if (j == 0) arg |= cmd.command << 24;

            if (j == 0 && i == to && (cmd.cmd == Command::Polygon || cmd.cmd == Command::Rectangle || cmd.cmd == Command::Line)) {
                arg = (cmd.command << 24) | 0x00ff00;
            }

            gpu->write(0, arg);
        }
    }
    gpu->gpuLogEnabled = true;
}

void dumpRegister(const char *name, uint32_t *reg) {
    ImGui::BulletText("%s", name);
    ImGui::NextColumn();
    ImGui::InputInt("", (int *)reg, 1, 100, ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::NextColumn();
}

void gteRegistersWindow(mips::gte::GTE gte) {
    if (!gteRegistersEnabled) {
        return;
    }
    ImGui::Begin("GTE registers", &gteRegistersEnabled);

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

    ImGui::Columns(3, nullptr, false);
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

void gteLogWindow(mips::CPU *cpu) {
    if (!gteLogEnabled) {
        return;
    }
    static char filterBuffer[16];
    static bool searchActive = false;
    bool filterActive = strlen(filterBuffer) > 0;
    ImGui::Begin("GTE Log", &gteLogEnabled);

    ImGui::BeginChild("GTE Log", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    for (int i = 0; i < cpu->gte.log.size(); i++) {
        auto ioEntry = cpu->gte.log[i];
        std::string t;
        if (ioEntry.mode == mips::gte::GTE::GTE_ENTRY::MODE::func) {
            t = string_format("%5d %c 0x%02x", i, 'F', ioEntry.n);
        } else {
            t = string_format("%5d %c %2d: 0x%08x", i, ioEntry.mode == mips::gte::GTE::GTE_ENTRY::MODE::read ? 'R' : 'W', ioEntry.n,
                              ioEntry.data);
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

void gpuLogWindow(mips::CPU *cpu) {
    if (!gpuLogEnabled) {
        return;
    }
    ImGui::Begin("GPU Log", &gpuLogEnabled);

    ImGui::BeginChild("GPU Log", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    int renderTo = -1;
    ImGuiListClipper clipper(cpu->getGPU()->gpuLogList.size());
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            auto &entry = cpu->getGPU()->gpuLogList[i];

            bool nodeOpen = ImGui::TreeNode((void *)(intptr_t)i, "cmd: 0x%02x  %s", entry.command, device::gpu::CommandStr[(int)entry.cmd]);

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

    ImGui::End();

    if (cpu->state != mips::CPU::State::run && renderTo >= 0) {
        replayCommands(cpu->getGPU(), renderTo);
    }
}

void ioWindow(mips::CPU *cpu) {
    if (!showIo) {
        return;
    }
    ImGui::Begin("IO", &showIo);

    ImGui::Columns(1, nullptr, false);
    ImGui::Text("Timer 0");

    ImGui::Columns(2, nullptr, false);
    dumpRegister("current", (uint32_t *)&cpu->timer0->current);
    dumpRegister("target", (uint32_t *)&cpu->timer0->target);
    dumpRegister("mode", (uint32_t *)&cpu->timer0->mode);

    ImGui::Columns(1, nullptr, false);
    ImGui::Text("Timer 1");

    ImGui::Columns(2, nullptr, false);
    dumpRegister("current", (uint32_t *)&cpu->timer1->current);
    dumpRegister("target", (uint32_t *)&cpu->timer1->target);
    dumpRegister("mode", (uint32_t *)&cpu->timer1->mode);

    ImGui::Columns(1, nullptr, false);
    ImGui::Text("Timer 2");

    ImGui::Columns(2, nullptr, false);
    dumpRegister("current", (uint32_t *)&cpu->timer2->current);
    dumpRegister("target", (uint32_t *)&cpu->timer2->target);
    dumpRegister("mode", (uint32_t *)&cpu->timer2->mode);

    ImGui::End();
}

void ioLogWindow(mips::CPU *cpu) {
#ifdef ENABLE_IO_LOG
    if (!ioLogEnabled) {
        return;
    }
    ImGui::Begin("IO Log", &ioLogEnabled);

    ImGui::BeginChild("IO Log", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImGuiListClipper clipper(cpu->ioLogList.size());
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            auto ioEntry = cpu->ioLogList[i];
            ImGui::Text("%c %2d 0x%08x: 0x%0*x %*s %s", ioEntry.mode == mips::CPU::IO_LOG_ENTRY::MODE::READ ? 'R' : 'W', ioEntry.size,
                        ioEntry.addr, ioEntry.size / 4, ioEntry.data,
                        // padding
                        8 - ioEntry.size / 4, "", mapIo(ioEntry.addr));
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
    ImGui::Image(ImTextureID(vramTextureId), currentSize);
    ImGui::End();
}

std::string formatOpcode(mips::Opcode &opcode) {
    auto disasm = debugger::decodeInstruction(opcode);
    return string_format("%s %*s %s", disasm.mnemonic.c_str(), 6 - disasm.mnemonic.length(), "",disasm.parameters.c_str());
}

void disassemblyWindow(mips::CPU *cpu) {
    static uint32_t startAddress = 0;
    static bool lockAddress = false;

    ImGui::Begin("Disassembly", &showDisassemblyWindow);

    if (ImGui::Button(cpu->state == mips::CPU::State::run ? "Pause" : "Run")) {
        if (cpu->state == mips::CPU::State::run)
            cpu->state = mips::CPU::State::pause;
        else
            cpu->state = mips::CPU::State::run;
    }
    ImGui::SameLine();
    if (ImGui::Button("Step")) {
        cpu->singleStep();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Map register names", &debugger::mapRegisterNames);

    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));
    ImGui::Columns(4);
    for (int i = 0; i < 34; i++) {
        if (i == 0)
            ImGui::Text("PC: 0x%08x", cpu->PC);
        else if (i == 32)
            ImGui::Text("hi: 0x%08x", cpu->hi);
        else if (i == 33)
            ImGui::Text("lo: 0x%08x", cpu->lo);
        else
            ImGui::Text("%s: 0x%08x", debugger::reg(i).c_str(), cpu->reg[i]);

        ImGui::NextColumn();
    }
    ImGui::Columns(1);

    ImGui::NewLine();

    ImGui::BeginChild("Disassembly", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), true);
    if (!lockAddress) startAddress = cpu->PC;
    for (int i = -15; i < 15; i++) {
        uint32_t address = (startAddress + i * 4) & 0xFFFFFFFC;
        mips::Opcode opcode(cpu->readMemory32(address));

        ImVec4 color = ImVec4(1.f, 1.f, 1.f, 1.f);
        if (i == 0) color = ImVec4(1.f, 1.f, 0.f, 1.f);
        else if (cpu->breakpoints.find(address) != cpu->breakpoints.end()) color = ImVec4(1.f, 0.f, 0.f, 1.f);
        ImGui::PushStyleColor(ImGuiCol_Text, color);

        if (ImGui::Selectable(string_format("0x%08x: %s", address, formatOpcode(opcode).c_str()).c_str())) {
            auto bp = cpu->breakpoints.find(address);
            if (bp == cpu->breakpoints.end()) {
                cpu->breakpoints.emplace(address, mips::CPU::Breakpoint());
            } else {
                cpu->breakpoints.erase(bp);
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

void breakpointsWindow(mips::CPU* cpu) {
    ImGui::Begin("Breakpoints", &showBreakpointsWindow);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));
    ImGui::BeginChild("Breakpoints", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), true);
    for (auto &bp : cpu->breakpoints) {
        mips::Opcode opcode(cpu->readMemory32(bp.first));

        ImVec4 color = ImVec4(1.f, 1.f, 1.f, 1.f);
        if (!bp.second.enabled) color = ImVec4(0.5f, 0.5f, 0.5f, 1.f);
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        
        if (ImGui::Selectable(string_format("0x%08x: %s (hit count: %d)", bp.first, formatOpcode(opcode).c_str(), bp.second.hitCount).c_str())) {
            bp.second.enabled = !bp.second.enabled;
        }

        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::End();
}

void ramWindow(mips::CPU *cpu) {
    static MemoryEditor editor;
    editor.DrawWindow("Ram", cpu->ram, mips::CPU::RAM_SIZE);
}