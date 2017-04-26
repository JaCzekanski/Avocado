#pragma once
#include <stdint.h>
#include "cop0.h"
#include "device/gpu.h"
#include "device/dma.h"
#include "device/cdrom.h"
#include "device/interrupt.h"
#include "device/timer.h"
#include "device/dummy.h"
#include "device/controller.h"
#include <unordered_map>
#include "device/spu.h"

/**
 * Accurate load delay slots, slightly slower.
 * Not sure, if games depends on that (assembler should nop delay slot)
 */
#define ENABLE_LOAD_DELAY_SLOTS

namespace bios {
struct Function;
}

namespace mips {
using namespace device;
/*
Based on http://problemkaputt.de/psx-spx.htm
0x00000000 - 0x20000000 is cache enabled
0x80000000 is mirrored to 0x00000000 (with cache)
0xA0000000 is mirrored to 0x00000000 (without cache)

00000000 - 00200000 2048K  RAM
1F000000 - 1F800000 8192K  Expansion ROM
1F800000 - 1F800400 1K     Scratchpad
1F801000 - 1F803000 8K     I/O
1F802000 - 1F804000 8K     Expansion 2
1FA00000 - 1FC00000 2048K  Expansion 3
1FC00000 - 1FC80000 512K   BIOS ROM
FFFE0000 - FFFE0100 0.5K   I/O (Cache Control)
*/
/*
r0      zero  - always 0
r1      at    - assembler temporary, reserved for use by assembler
r2-r3   v0-v1 - (value) returned by subroutine
r4-r7   a0-a3 - (arguments) first four parameters for a subroutine
r8-r15  t0-t7 - (temporaries) subroutines may use without saving
r24-r25 t8-t9
r16-r23 s0-s7 - saved temporaries, must be saved
r26-r27 k0-k1 - Reserved for use by interrupt/trap handler
r28     gp    - global pointer - used for accessing static/extern variables
r29     sp    - stack pointer
r30     fp    - frame pointer
r31     ra    - return address
*/

struct LoadSlot {
    uint32_t reg;
    uint32_t data;
    uint32_t prevData;
};

struct CPU {
    static const int REGISTER_COUNT = 32;
    static const int BIOS_SIZE = 512 * 1024;
    static const int RAM_SIZE = 2 * 1024 * 1024;
    static const int SCRATCHPAD_SIZE = 1024;
    static const int EXPANSION_SIZE = 1 * 1024 * 1024;
    enum class State {
        halted,  // Cannot be run until reset
        stop,    // after reset
        pause,   // if debugger attach
        run      // normal state
    } state
        = State::stop;

    uint32_t PC;
    uint32_t jumpPC;
    bool shouldJump;
    uint32_t reg[32];
    cop0::COP0 cop0;
    uint32_t hi, lo;
    bool exception;

    uint8_t bios[BIOS_SIZE];
    uint8_t ram[RAM_SIZE];
    uint8_t scratchpad[SCRATCHPAD_SIZE];
    uint8_t expansion[EXPANSION_SIZE];

	bool debugOutput = false; // Print BIOS logs
   public:
    // Devices
    interrupt::Interrupt *interrupt = nullptr;
    controller::Controller *controller = nullptr;
    cdrom::CDROM *cdrom = nullptr;
    timer::Timer *timer0 = nullptr;
    timer::Timer *timer1 = nullptr;
    timer::Timer *timer2 = nullptr;
    dma::DMA *dma = nullptr;
    spu::SPU *spu = nullptr;

   private:
    Dummy *memoryControl = nullptr;
    Dummy *serial = nullptr;
    gpu::GPU *gpu = nullptr;
    Dummy *mdec = nullptr;
    Dummy *expansion2 = nullptr;

    uint8_t readMemory(uint32_t address);
    void writeMemory(uint32_t address, uint8_t data);
    void checkForInterrupts();
    void handleBiosFunction();
    void moveLoadDelaySlots();

   public:
    CPU();

    void setGPU(gpu::GPU *gpu) {
        this->gpu = gpu;
        dma->setGPU(gpu);
    }

    LoadSlot slots[2];
    void loadDelaySlot(int r, uint32_t data);
    uint8_t readMemory8(uint32_t address);
    uint16_t readMemory16(uint32_t address);
    uint32_t readMemory32(uint32_t address);
    void writeMemory8(uint32_t address, uint8_t data);
    void writeMemory16(uint32_t address, uint16_t data);
    void writeMemory32(uint32_t address, uint32_t data);
    void printFunctionInfo(int type, uint8_t number, bios::Function f);
    bool executeInstructions(int count);
    void emulateFrame();

    // Helpers
    bool biosLog = true;
    bool printStackTrace = false;
    bool disassemblyEnabled = false;
    char *_mnemonic = (char *)"";
    std::string _disasm;
    bool loadBios(std::string name);
    bool loadExpansion(std::string name);
    bool loadExeFile(std::string exePath);
    void dumpRam();
};
};
