#include "cpu.h"
#include <fmt/core.h>
#include <imgui.h>
#include <algorithm>
#include "debugger/debugger.h"
#include "system.h"
#include "utils/address.h"

namespace gui::debug {

struct Segment {
    const char* name;
    uint32_t base;
    uint32_t size;

    bool inRange(uint32_t addr) { return addr >= base && addr < base + size; }

    static Segment fromAddress(uint32_t);
};

namespace segments {
Segment RAM = {"RAM", System::RAM_BASE, System::RAM_SIZE};
Segment EXPANSION = {"EXPANSION", System::EXPANSION_BASE, System::EXPANSION_SIZE};
Segment SCRATCHPAD = {"SCRATCHPAD", System::SCRATCHPAD_BASE, System::SCRATCHPAD_SIZE};
Segment IO = {"IO", System::IO_BASE, System::IO_SIZE};
Segment BIOS = {"BIOS", System::BIOS_BASE, System::BIOS_SIZE};
Segment IOCONTROL = {"IOCONTROL", 0xfffe0130, 4};
Segment UNKNOWN = {"UNKNOWN", 0, 0};
};  // namespace segments

Segment Segment::fromAddress(uint32_t address) {
    uint32_t addr = align_mips<uint32_t>(address);

    if (addr >= segments::RAM.base && addr < segments::RAM.base + segments::RAM.size * 4) {
        return segments::RAM;
    }
    if (segments::EXPANSION.inRange(addr)) {
        return segments::EXPANSION;
    }
    if (segments::SCRATCHPAD.inRange(addr)) {
        return segments::SCRATCHPAD;
    }
    if (segments::BIOS.inRange(addr)) {
        return segments::BIOS;
    }
    if (segments::IO.inRange(addr)) {
        return segments::IO;
    }
    if (segments::IOCONTROL.inRange(address)) {
        return segments::IOCONTROL;
    }

    return segments::UNKNOWN;
}

std::string formatOpcode(mips::Opcode& opcode) {
    auto disasm = debugger::decodeInstruction(opcode);
    return fmt::format("{} {:{}c} {}", disasm.mnemonic, ' ', std::max(0, 6 - (int)disasm.mnemonic.length()), disasm.parameters);
}

void CPU::debuggerWindow(System* sys) {
    bool goToPc = false;
    ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin("Debugger", &debuggerWindowOpen, ImGuiWindowFlags_NoScrollbar);

    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);

    if (ImGui::Button(sys->state == System::State::run ? "Pause" : "Run")) {
        if (sys->state == System::State::run)
            sys->state = System::State::pause;
        else
            sys->state = System::State::run;
    }
    ImGui::SameLine();
    if (ImGui::Button("Step in")) {
        sys->singleStep();
    }
    ImGui::SameLine();
    if (ImGui::Button("Step over")) {
        sys->cpu->breakpoints.emplace(sys->cpu->PC + 4, mips::CPU::Breakpoint(true));
        sys->state = System::State::run;
    }
    ImGui::SameLine();
    if (ImGui::Button("Go to PC")) {
        goToPc = true;
    }

    ImGui::SameLine();
    ImGui::Checkbox("Follow PC", &debugger::followPC);
    ImGui::SameLine();
    ImGui::Checkbox("Map register names", &debugger::mapRegisterNames);

    ImGui::Separator();

    auto glyphSize = ImGui::CalcTextSize("F").x + 1;  // We assume the font is mono-space
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const int col_num = 4;
        std::vector<float> columnsWidth(col_num);
        unsigned int n = 0;

        auto column = [&](const char* reg, uint32_t val) {
            auto width = glyphSize * 13 + 8;
            if (width > columnsWidth[n]) columnsWidth[n] = width;

            ImGui::TextUnformatted(reg);
            ImGui::SameLine();

            auto color = ImVec4(1.f, 1.f, 1.f, (val == 0) ? 0.25f : 1.f);
            ImGui::TextColored(color, "0x%08x", val);
            ImGui::NextColumn();

            if (++n >= columnsWidth.size()) n = 0;
        };

        ImGui::Columns(col_num, nullptr, false);

        column(" pc: ", sys->cpu->PC);
        ImGui::NextColumn();
        column(" hi: ", sys->cpu->hi);
        column(" lo: ", sys->cpu->lo);

        ImGui::NextColumn();
        for (int i = 1; i < 32; i++) {
            column(fmt::format("{:>3}: ", debugger::reg(i)).c_str(), sys->cpu->reg[i]);
        }

        for (int c = 0; c < col_num; c++) {
            ImGui::SetColumnWidth(c, columnsWidth[c]);
        }
        ImGui::Columns(1);
        ImGui::PopStyleVar();
    }

    ImGui::NewLine();

    ImGui::BeginChild("##scrolling", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()));
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    auto segment = Segment::fromAddress(sys->cpu->PC);  // <-- do that only when following pc
    ImGuiListClipper clipper(segment.size / 4);

    uint32_t base = segment.base & ~0xe0000000;
    base |= sys->cpu->PC & 0xe0000000;

    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            uint32_t address = base + i * 4;

            mips::Opcode opcode(sys->readMemory32(address));
            auto disasm = debugger::decodeInstruction(opcode);

            int xStart = glyphSize * 3;
            if (disasm.isBranch()) {
                auto color = IM_COL32(255, 255, 255, 192);
                auto lineHeight = ImGui::GetTextLineHeight();
                int16_t branchOffset = disasm.opcode.offset;

                int xEnd = xStart - clamp(abs(branchOffset), (int)glyphSize, xStart);

                ImVec2 src = ImGui::GetCursorScreenPos();
                src.y += lineHeight / 2;
                // Compensate for Branch Delay
                src.y += lineHeight;

                ImVec2 dst = src;
                dst.y += branchOffset * lineHeight;

                // From
                drawList->AddLine(ImVec2(src.x + xStart, src.y), ImVec2(src.x + xEnd, src.y), color);

                // Vertical line
                drawList->AddLine(ImVec2(src.x + xEnd, src.y), ImVec2(dst.x + xEnd, dst.y), color);

                // To
                drawList->AddLine(ImVec2(dst.x + xStart, dst.y), ImVec2(dst.x + xEnd, dst.y), color);

                // Arrow
                drawList->AddTriangleFilled(ImVec2(dst.x + xStart, dst.y), ImVec2(dst.x + xStart - 3, dst.y - 3),
                                            ImVec2(dst.x + xStart - 3, dst.y + 3), color);
            }

            bool breakpointActive = sys->cpu->breakpoints.find(address) != sys->cpu->breakpoints.end();
            if (breakpointActive) {
                const float size = 4.f;
                ImVec2 src = ImGui::GetCursorScreenPos();
                src.x += size;
                src.y += ImGui::GetTextLineHeight() / 2;
                drawList->AddCircleFilled(src, size, IM_COL32(255, 0, 0, 255));
            }

            bool isCurrentPC = address == sys->cpu->PC;
            if (isCurrentPC) {
                auto color = IM_COL32(255, 255, 0, 255);
                const float size = 4;
                ImVec2 src = ImGui::GetCursorScreenPos();
                src.y += ImGui::GetTextLineHeight() / 2;

                // Arrow
                drawList->AddTriangleFilled(ImVec2(src.x + xStart, src.y), ImVec2(src.x + xStart - size, src.y - size),
                                            ImVec2(src.x + xStart - size, src.y + size), color);

                // Line
                drawList->AddRectFilled(ImVec2(src.x + xStart - size * 3, src.y + size / 2),
                                        ImVec2(src.x + xStart - size, src.y - size / 2), color);
            }

            std::string comment = "";
            ImU32 color = IM_COL32(255, 255, 255, 255);
            if (isCurrentPC) {
                color = IM_COL32(255, 255, 0, 255);
            } else if (breakpointActive) {
                color = IM_COL32(255, 0, 0, 255);
            } else if (!disasm.valid) {
                comment = "; invalid instruction";
                color = IM_COL32(255, 255, 255, 64);
            } else if (disasm.opcode.opcode == 0) {  // NOP
                color = IM_COL32(255, 255, 255, 64);
            }
            ImGui::PushStyleColor(ImGuiCol_Text, color);

            auto line
                = fmt::format("{} {:{}c} {}", disasm.mnemonic, ' ', std::max(0, 6 - (int)disasm.mnemonic.length()), disasm.parameters);
            if (ImGui::Selectable(fmt::format("    {}:0x{:08x}: {} {:{}c} {}", segment.name, address, line, ' ',
                                              std::max(0, 25 - (int)line.length()), comment)
                                      .c_str())) {
                auto bp = sys->cpu->breakpoints.find(address);
                if (bp == sys->cpu->breakpoints.end()) {
                    sys->cpu->breakpoints.emplace(address, mips::CPU::Breakpoint());
                } else {
                    sys->cpu->breakpoints.erase(bp);
                }
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGui::GetIO().MouseClicked[1])) {
                ImGui::OpenPopup("##instruction_options");
                contextMenuAddress = address;
            }

            ImGui::PopStyleColor();

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%02x %02x %02x %02x", opcode.opcode & 0xff, (opcode.opcode >> 8) & 0xff, (opcode.opcode >> 16) & 0xff,
                                  (opcode.opcode >> 24) & 0xff);
            }
        }
    }
    ImGui::PopStyleVar(2);
    ImGui::EndChild();

    if (ImGui::BeginPopupContextItem("##instruction_options")) {
        auto bp = sys->cpu->breakpoints.find(contextMenuAddress);
        auto breakpointExist = bp != sys->cpu->breakpoints.end();

        if (ImGui::Selectable("Run to line")) {
            sys->cpu->breakpoints.emplace(contextMenuAddress, mips::CPU::Breakpoint(true));
            sys->state = System::State::run;
        }

        if (breakpointExist && ImGui::Selectable("Remove breakpoint")) sys->cpu->breakpoints.erase(bp);
        if (!breakpointExist && ImGui::Selectable("Add breakpoint"))
            sys->cpu->breakpoints.emplace(contextMenuAddress, mips::CPU::Breakpoint());

        ImGui::EndPopup();
    }

    ImGui::Text("Go to address ");
    ImGui::SameLine();

    ImGui::PushItemWidth(80.f);

    bool doScroll = false;
    goToAddr = 0;

    if (ImGui::InputText("##addr", addrInputBuffer, 32, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (sscanf(addrInputBuffer, "%x", &goToAddr) == 1) {
            goToAddr = (goToAddr - base) / 4;
            doScroll = true;
            debugger::followPC = false;
            // TODO: Change segment if necessary
        }
    }
    ImGui::PopItemWidth();

    if (!doScroll && (goToPc || (debugger::followPC && sys->cpu->PC != prevPC))) {
        prevPC = sys->cpu->PC;
        goToAddr = ((sys->cpu->PC - base) / 4);
        doScroll = true;
    }

    if (doScroll) {
        auto px = goToAddr * ImGui::GetTextLineHeight();
        ImGui::BeginChild("##scrolling");
        ImGui::SetScrollFromPosY(ImGui::GetCursorStartPos().y + px);
        ImGui::EndChild();
    }

    ImGui::PopFont();

    ImGui::End();
}

void CPU::breakpointsWindow(System* sys) {
    static uint32_t selectedBreakpoint = 0;
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Breakpoints", &breakpointsWindowOpen);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));
    ImGui::BeginChild("Breakpoints", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), true);
    for (auto& bp : sys->cpu->breakpoints) {
        mips::Opcode opcode(sys->readMemory32(bp.first));

        ImVec4 color = ImVec4(1.f, 1.f, 1.f, 1.f);
        if (!bp.second.enabled) color = ImVec4(0.5f, 0.5f, 0.5f, 1.f);
        ImGui::PushStyleColor(ImGuiCol_Text, color);

        if (ImGui::Selectable(fmt::format("0x{:08x}: {} (hit count: {})", bp.first, formatOpcode(opcode), bp.second.hitCount).c_str())) {
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
        static char addressInput[10];
        uint32_t address;
        ImGui::Text("Address: ");
        ImGui::SameLine();

        ImGui::PushItemWidth(80);
        if (ImGui::InputText("", addressInput, 10, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)
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

void CPU::watchWindow(System* sys) {
    static int selectedWatch = -1;

    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Watch", &watchWindowOpen);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));
    ImGui::BeginChild("Watch", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), true);
    int i = 0;
    for (auto& watch : watches) {
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
        static const char* sizeLabels[] = {"1 byte (8 bits)", "2 bytes (16 bits)", "4 bytes (32 bits)"};
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

void CPU::ramWindow(System* sys) {
    editor.Open = ramWindowOpen;
    editor.DrawWindow("Ram", sys->ram.data(), System::RAM_SIZE);
    ramWindowOpen = editor.Open;
}

void CPU::displayWindows(System* sys) {
    if (debuggerWindowOpen) debuggerWindow(sys);
    if (breakpointsWindowOpen) breakpointsWindow(sys);
    if (watchWindowOpen) watchWindow(sys);
    if (ramWindowOpen) ramWindow(sys);
}
}  // namespace gui::debug