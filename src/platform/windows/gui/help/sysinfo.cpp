// Reference : https://stackoverflow.com/questions/6121792/how-to-check-if-a-cpu-supports-the-sse3-instruction-set
#if defined(_M_IX86) || defined(_M_X64)
#include "sysinfo.h"
#include <imgui.h>
#include <intrin.h>
#include <cstdint>

namespace gui::help {

void SysInfo::sysinfoWindow() {
    ImGui::Begin("System Information", &sysinfoWindowOpen, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("%s", cpu_info.brand_string);
    if (cpu_info.bLongMode)
        ImGui::Text("Your CPU have 64-bit");
    else
        ImGui::Text("Your CPU have 32 bit");
    ImGui::End();
}

void SysInfo::displayWindows() {
    if (sysinfoWindowOpen) sysinfoWindow();
}

void do_cpuidex(uint32_t regs[4], uint32_t cpuid_leaf, uint32_t ecxval) { __cpuidex((int *)regs, cpuid_leaf, ecxval); }
void do_cpuid(uint32_t regs[4], uint32_t cpuid_leaf) { __cpuid((int *)regs, cpuid_leaf); }

CPUInfo cpu_info;

CPUInfo::CPUInfo() { 
	Detect(); 
}

// Detects the various cpu features
void CPUInfo::Detect() { 
	memset(this, 0, sizeof(*this));
#ifdef _M_IX86
    Mode64bit = false;
#elif defined(_M_X64)
    Mode64bit = true;
    OS64bit = true;
#endif

    // Set obvious defaults, for extra safety
    if (Mode64bit) {
        bLongMode = true;
    }

    // Assume CPU supports the CPUID instruction. Those that don't can barely
    // boot modern OS:es anyway.
    uint32_t cpu_id[4];
    memset(cpu_string, 0, sizeof(cpu_string));

    // Detect CPU's CPUID capabilities, and grab cpu string
    do_cpuid(cpu_id, 0x00000000);
    uint32_t max_std_fn = cpu_id[0];  // EAX
    *((int *)cpu_string) = cpu_id[1];
    *((int *)(cpu_string + 4)) = cpu_id[3];
    *((int *)(cpu_string + 8)) = cpu_id[2];
    do_cpuid(cpu_id, 0x80000000);
    uint32_t max_ex_fn = cpu_id[0];
    if (!strcmp(cpu_string, "GenuineIntel"))
        vendor = VENDOR_INTEL;
    else if (!strcmp(cpu_string, "AuthenticAMD"))
        vendor = VENDOR_AMD;

    // Set reasonable default brand string even if brand string not available.
    strcpy(brand_string, cpu_string);

	// Detect family and other misc stuff.
    if (max_std_fn >= 1) {
        do_cpuid(cpu_id, 0x00000001);
        int family = ((cpu_id[0] >> 8) & 0xf) + ((cpu_id[0] >> 20) & 0xff);
        int model = ((cpu_id[0] >> 4) & 0xf) + ((cpu_id[0] >> 12) & 0xf0);
        // Detect people unfortunate enough to be on an Atom
        if (family == 6
            && (model == 0x1C || model == 0x26 || model == 0x27 || model == 0x35 || model == 0x36 || model == 0x37 || model == 0x4A
                || model == 0x4D || model == 0x5A || model == 0x5D))
            bAtom = true;
    }
	if (max_ex_fn >= 0x80000004) {
        // Extract brand string
        do_cpuid(cpu_id, 0x80000002);
        memcpy(brand_string, cpu_id, sizeof(cpu_id));
        do_cpuid(cpu_id, 0x80000003);
        memcpy(brand_string + 16, cpu_id, sizeof(cpu_id));
        do_cpuid(cpu_id, 0x80000004);
        memcpy(brand_string + 32, cpu_id, sizeof(cpu_id));
    }

	if (max_ex_fn >= 0x80000001) {
		// Check for more features.
		//This check to detect CPU 64 bit support even though you're run on x86 build
        do_cpuid(cpu_id, 0x80000001);
        if ((cpu_id[3] >> 29) & 1) bLongMode = true;
	}
    }
}  // namespace gui::help
#endif // defined(_M_IX86) || defined(_M_X64)