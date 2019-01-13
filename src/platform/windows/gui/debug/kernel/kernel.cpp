#include "kernel.h"
#include <imgui.h>

bool showKernelWindow = false;

namespace PCB {
bool getThreadNumberForAddress(System* sys, uint32_t ptr) {
    const uint32_t tcb_size = 0xC0;
    uint32_t tcb_addr = sys->readMemory32(0x110);
    // TODO: mask?

    int number = (ptr - tcb_addr) / tcb_size;

    return number;
}
void parse(System* sys) {
    const uint32_t size = 0x04;
    uint32_t addr = sys->readMemory32(0x108);
    uint32_t count = sys->readMemory32(0x108 + 4) / size;

    ImGui::TreePush();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::Text("(addr: 0x%08x, count: 0x%02x)", addr, count);
    ImGui::Separator();

    for (auto i = 0u; i < count; i++) {
        uint32_t ptr = sys->readMemory32(addr + i * size + 0);
        ImGui::Text("Process  %d", i);
        ImGui::Text("ptr      0x%08x", ptr);
        ImGui::Text("thread   %d", getThreadNumberForAddress(sys, ptr));
        ImGui::Separator();
    }
    ImGui::PopStyleVar();
    ImGui::TreePop();
}
}  // namespace PCB

namespace TCB {

void parse(System* sys) {
    const uint32_t size = 0xC0;
    uint32_t addr = sys->readMemory32(0x110);
    uint32_t count = sys->readMemory32(0x110 + 4) / size;

    ImGui::TreePush();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::Text("(addr: 0x%08x, count: 0x%02x)", addr, count);
    ImGui::Separator();

    for (auto i = 0u; i < count; i++) {
        uint32_t status = sys->readMemory32(addr + i * size + 0);
        uint32_t r[32];
        for (int j = 0; j < 32; j++) {
            r[j] = sys->readMemory32(addr + i * size + 8 + j * 4);
        }
        uint32_t epc = sys->readMemory32(addr + i * size + 136);
        uint32_t hi = sys->readMemory32(addr + i * size + 140);
        uint32_t lo = sys->readMemory32(addr + i * size + 144);
        uint32_t sr = sys->readMemory32(addr + i * size + 148);
        uint32_t cause = sys->readMemory32(addr + i * size + 152);

        if (status == 0x1000) continue;

        ImGui::Text("Thread  %d", i);
        for (int j = 0; j < 32; j++) {
            ImGui::Text("r%2d   0x%08x", j, r[j]);
        }
        ImGui::Text("epc   0x%08x", epc);
        ImGui::Text("hi    0x%08x", hi);
        ImGui::Text("lo    0x%08x", lo);
        ImGui::Text("sr    0x%08x", sr);
        ImGui::Text("sr    0x%08x", sr);
        ImGui::Text("cause 0x%08x", cause);
        ImGui::Separator();
    }
    ImGui::PopStyleVar();
    ImGui::TreePop();
}
}  // namespace TCB

namespace EvCB {

const char* mapClass(uint32_t clazz) {
    switch (clazz) {
        case 0xF0000001: return "VBLANK";
        case 0xF0000002: return "GPU";
        case 0xF0000003: return "CDROM";
        case 0xF0000004: return "DMA";
        case 0xF0000005: return "Timer0";
        case 0xF0000006: return "Timer1/2";
        case 0xF0000007: return "N/A";
        case 0xF0000008: return "Controller";
        case 0xF0000009: return "SPU";
        case 0xF000000A: return "PIO";
        case 0xF000000B: return "SIO";
        case 0xF0000010: return "Exception";
        case 0xF0000011: return "Memory card (BIOS)";
        case 0xF2000000: return "Root counter 0";
        case 0xF2000001: return "Root counter 1";
        case 0xF2000002: return "Root counter 2";
        case 0xF2000003: return "Root counter 3";
        case 0xF4000001: return "Memory card (BIOS)";
        case 0xF4000002: return "libmath (BIOS)";
        default: return "???";
    }
};

const char* mapStatus(uint32_t status) {
    switch (status) {
        case 0x1000: return "disabled";
        case 0x2000: return "enabled/busy";
        case 0x4000: return "enabled/ready";
        default: return "???";
    }
};

const char* mapSpec(uint32_t spec) {
    switch (spec) {
        case 0x0001: return "counter becomes zero";
        case 0x0002: return "interrupted";
        case 0x0004: return "end of i/o";
        case 0x0008: return "file was closed";
        case 0x0010: return "command acknowledged";
        case 0x0020: return "command completed";
        case 0x0040: return "data ready";
        case 0x0080: return "data end";
        case 0x0100: return "time out";
        case 0x0200: return "unknown command";
        case 0x0400: return "end of read buffer";
        case 0x0800: return "end of write buffer";
        case 0x1000: return "general interrupt";
        case 0x2000: return "new device";
        case 0x4000: return "system call instruction";
        case 0x8000: return "error happened";
        case 0x8001: return "previous write error happened";
        case 0x0301: return "domain error in libmath";
        case 0x0302: return "range error in libmath";
        default: return "???";
    }
};

const char* mapMode(uint32_t mode) {
    switch (mode) {
        case 0x1000: return "Execute callback/stay busy";
        case 0x2000: return "No callback/mark ready";
        default: return "???";
    }
};

void parse(System* sys) {
    const uint32_t size = 0x1c;
    uint32_t addr = sys->readMemory32(0x120);
    uint32_t count = sys->readMemory32(0x120 + 4) / size;

    ImGui::TreePush();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::Text("(addr: 0x%08x, count: 0x%02x)", addr, count);
    ImGui::Separator();

    for (auto i = 0u; i < count; i++) {
        uint32_t clazz = sys->readMemory32(addr + i * size + 0);
        uint32_t status = sys->readMemory32(addr + i * size + 4);
        uint32_t spec = sys->readMemory32(addr + i * size + 8);
        uint32_t mode = sys->readMemory32(addr + i * size + 12);
        uint32_t ptr = sys->readMemory32(addr + i * size + 16);

        if (status == 0) continue;

        ImGui::Text("Event  %d", i);
        ImGui::Text("Class  0x%08x (%s)", clazz, EvCB::mapClass(clazz));
        ImGui::Text("Status 0x%08x (%s)", status, EvCB::mapStatus(status));
        ImGui::Text("Spec   0x%08x (%s)", spec, EvCB::mapSpec(spec));
        ImGui::Text("Mode   0x%08x (%s)", mode, EvCB::mapMode(mode));
        if (mode == 0x1000) {
            ImGui::Text("Ptr    0x%08x", ptr);
        }
        ImGui::Separator();
    }
    ImGui::PopStyleVar();
    ImGui::TreePop();
}
}  // namespace EvCB

void kernelWindow(System* sys) {
    ImGui::Begin("Kernel", &showKernelWindow, ImVec2(300, 200));
    int treeFlags = ImGuiTreeNodeFlags_CollapsingHeader;

    if (ImGui::TreeNodeEx("Process Control Blocks (PCB)", treeFlags)) PCB::parse(sys);
    if (ImGui::TreeNodeEx("Thread Control Blocks (TCB)", treeFlags)) TCB::parse(sys);
    if (ImGui::TreeNodeEx("Event Control Blocks (EvCB)", treeFlags)) EvCB::parse(sys);

    ImGui::End();
}