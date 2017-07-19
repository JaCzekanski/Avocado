#pragma once
#include <stdint.h>
#include "cop0.h"
#include "gte.h"
#include "device/gpu.h"
#include "device/dma.h"
#include "device/cdrom.h"
#include "device/interrupt.h"
#include "device/timer.h"
#include "device/dummy.h"
#include "device/controller.h"
#include "device/mdec.h"
#include "device/spu.h"

#include <memory>
#include <vector>

/**
 * NOTE:
 * Build flags are configured with Premake5 build system
 */

/**
 * #define ENABLE_LOAD_DELAY_SLOTS
 * Switch: --disable-load-delay-slots
 * Default: true
 *
 * Accurate load delay slots, slightly slower.
 * Not sure, if games depends on that (assembler should nop delay slot)
 */

/**
 * #define ENABLE_BREAKPOINTS
 * Switch: --enable-breakpoints
 * Default: false
 *
 * Enables logic that is responsible for checking code breakpoints.
 * Used in auto test build
 */

/**
 * #define ENABLE_IO_LOG
 * Switch --enable-io-log
 * Default: false
 *
 * Enables IO access buffer log
 */

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
r16-r23 s0-s7 - saved temporaries, must be saved
r24-r25 t8-t9
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
    enum class State {
        halted,  // Cannot be run until reset
        stop,    // after reset
        pause,   // if debugger attach
        run      // normal state
    };

    static const int REGISTER_COUNT = 32;
    static const int BIOS_SIZE = 512 * 1024;
    static const int RAM_SIZE = 2 * 1024 * 1024;
    static const int SCRATCHPAD_SIZE = 1024;
    static const int EXPANSION_SIZE = 1 * 1024 * 1024;
    State state = State::stop;

    uint32_t PC;
    uint32_t jumpPC;
    bool shouldJump;
    uint32_t reg[REGISTER_COUNT];
    cop0::COP0 cop0;
    gte::GTE gte;
    uint32_t hi, lo;
    bool exception;

    uint8_t bios[BIOS_SIZE];
    uint8_t ram[RAM_SIZE];
    uint8_t scratchpad[SCRATCHPAD_SIZE];
    uint8_t expansion[EXPANSION_SIZE];

    bool debugOutput = true;  // Print BIOS logs
   public:
    // Devices
    std::unique_ptr<interrupt::Interrupt> interrupt;
    std::unique_ptr<controller::Controller> controller;
    std::unique_ptr<cdrom::CDROM> cdrom;
    std::unique_ptr<dma::DMA> dma;
    std::unique_ptr<spu::SPU> spu;

    std::unique_ptr<Dummy> memoryControl;
    std::unique_ptr<Dummy> serial;
    std::unique_ptr<gpu::GPU> gpu;
    std::unique_ptr<timer::Timer> timer0;
    std::unique_ptr<timer::Timer> timer1;
    std::unique_ptr<timer::Timer> timer2;
    std::unique_ptr<mdec::MDEC> mdec;
    std::unique_ptr<Dummy> expansion2;

    uint8_t readMemory(uint32_t address);
    void writeMemory(uint32_t address, uint8_t data);
    void checkForInterrupts();
    void handleBiosFunction();
    void moveLoadDelaySlots();

   public:
    CPU();

    LoadSlot slots[2];
    void loadDelaySlot(uint32_t r, uint32_t data);
    uint8_t readMemory8(uint32_t address);
    uint16_t readMemory16(uint32_t address);
    uint32_t readMemory32(uint32_t address);
    void writeMemory8(uint32_t address, uint8_t data);
    void writeMemory16(uint32_t address, uint16_t data);
    void writeMemory32(uint32_t address, uint32_t data);
    void printFunctionInfo(int type, uint8_t number, bios::Function f);
    bool executeInstructions(int count);
    void emulateFrame();
    gpu::GPU *getGPU() const { return gpu.get(); }
    void softReset();

    // Helpers
    bool biosLog = false;
    bool printStackTrace = false;
    bool disassemblyEnabled = false;
    char *_mnemonic = (char *)"";
    std::string _disasm;
    bool loadBios(std::string name);
    bool loadExpansion(std::string name);
    bool loadExeFile(std::string exePath);
    void dumpRam();
#ifdef ENABLE_IO_LOG
    struct IO_LOG_ENTRY {
        enum class MODE { READ, WRITE } mode;
        uint32_t size;
        uint32_t addr;
        uint32_t data;
    };
    std::vector<IO_LOG_ENTRY> ioLogList;
#endif
};
};
