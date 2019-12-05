#include "io.h"
#include <imgui.h>
#include "system.h"

namespace gui::debug {

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

void IO::logWindow(System *sys) {
#ifdef ENABLE_IO_LOG
    ImGui::SetNextWindowSize(ImVec2(200, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("IO Log", &logWindowOpen);

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

void IO::displayWindows(System *sys) {
    if (logWindowOpen) logWindow(sys);
}
}  // namespace gui::debug