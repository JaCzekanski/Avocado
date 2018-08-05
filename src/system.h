#pragma once
#include <cstdint>
#include "cpu/cpu.h"
#include "device/cdrom.h"
#include "device/controller.h"
#include "device/dma.h"
#include "device/expansion2.h"
#include "device/gpu/gpu.h"
#include "device/interrupt.h"
#include "device/mdec.h"
#include "device/memory_control.h"
#include "device/serial.h"
#include "device/spu/spu.h"
#include "device/timer.h"
#include "utils/macros.h"

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

struct System {
    enum class State {
        halted,  // Cannot be run until reset
        stop,    // after reset
        pause,   // if debugger attach
        run      // normal state
    };

    static const int BIOS_BASE = 0x1fc00000;
    static const int RAM_BASE = 0x00000000;
    static const int SCRATCHPAD_BASE = 0x1f800000;
    static const int EXPANSION_BASE = 0x1f000000;
    static const int IO_BASE = 0x1f801000;

    static const int BIOS_SIZE = 512 * 1024;
    static const int RAM_SIZE = 2 * 1024 * 1024;
    static const int SCRATCHPAD_SIZE = 1024;
    static const int EXPANSION_SIZE = 1 * 1024 * 1024;
    static const int IO_SIZE = 0x2000;
    State state = State::stop;

    uint8_t bios[BIOS_SIZE];
    uint8_t ram[RAM_SIZE];
    uint8_t scratchpad[SCRATCHPAD_SIZE];
    uint8_t expansion[EXPANSION_SIZE];

    bool debugOutput = true;  // Print BIOS logs

    // Devices
    std::unique_ptr<mips::CPU> cpu;

    std::unique_ptr<device::cdrom::CDROM> cdrom;
    std::unique_ptr<device::controller::Controller> controller;
    std::unique_ptr<device::dma::DMA> dma;
    std::unique_ptr<Expansion2> expansion2;
    std::unique_ptr<GPU> gpu;
    std::unique_ptr<Interrupt> interrupt;
    std::unique_ptr<MDEC> mdec;
    std::unique_ptr<MemoryControl> memoryControl;
    std::unique_ptr<spu::SPU> spu;
    std::unique_ptr<Serial> serial;
    std::unique_ptr<Timer<0>> timer0;
    std::unique_ptr<Timer<1>> timer1;
    std::unique_ptr<Timer<2>> timer2;

    template <typename T>
    INLINE T readMemory(uint32_t address);
    template <typename T>
    INLINE void writeMemory(uint32_t address, T data);
    void singleStep();
    void handleBiosFunction();

    System();
    uint8_t readMemory8(uint32_t address);
    uint16_t readMemory16(uint32_t address);
    uint32_t readMemory32(uint32_t address);
    void writeMemory8(uint32_t address, uint8_t data);
    void writeMemory16(uint32_t address, uint16_t data);
    void writeMemory32(uint32_t address, uint32_t data);
    void printFunctionInfo(int type, uint8_t number, bios::Function f);
    void emulateFrame();
    void softReset();

    // Helpers
    int biosLog = 0;
    bool printStackTrace = false;
    bool loadBios(std::string name);
    bool loadExpansion(const std::vector<uint8_t>& _exe);
    bool loadExeFile(const std::vector<uint8_t>& _exe);
    void dumpRam();

#ifdef ENABLE_IO_LOG
    struct IO_LOG_ENTRY {
        enum class MODE { READ, WRITE } mode;

        uint32_t size;
        uint32_t addr;
        uint32_t data;
        uint32_t pc;
    };

    std::vector<IO_LOG_ENTRY> ioLogList;
#endif
};