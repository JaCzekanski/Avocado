#pragma once
#include <cstdint>
#include <unordered_map>
#include "cpu/cop0.h"
#include "cpu/gte/gte.h"
#include "opcode.h"
#include "utils/macros.h"

struct System;

namespace mips {

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

    template <class Archive>
    void serialize(Archive& ar) {
        ar(reg, data, prevData);
    }
};

struct CPU {
    inline static const int REGISTER_COUNT = 32;

    // Saved state for exception handling
    uint32_t exceptionPC;
    bool exceptionIsInBranchDelay;
    bool exceptionIsBranchTaken;

    uint32_t PC;         // Address to be executed
    uint32_t nextPC;     // Next address to be executed
    bool inBranchDelay;  // Is CPU currently in Branch Delay slot
    bool branchTaken;    // If CPU is in Branch Delay slot, was the branch taken
    LoadSlot slots[2];   // Load Delay slots

    uint32_t reg[REGISTER_COUNT];
    uint32_t hi, lo;
    COP0 cop0;
    GTE gte;
    System* sys;
    Opcode _opcode;

    CPU(System* sys);
    void checkForInterrupts();
    void loadDelaySlot(uint32_t r, uint32_t data);
    void moveLoadDelaySlots();
    INLINE void loadAndInvalidate(uint32_t r, uint32_t data) {
        if (r == 0) return;
        reg[r] = data;

        // Invalidate
        if (slots[0].reg == r) {
            slots[0].reg = 0;
        }
    }
    INLINE void jump(uint32_t address) {
        nextPC = address;
        branchTaken = true;
    }
    INLINE void setPC(uint32_t address) {
        PC = address;
        nextPC = address + 4;
    }

    void saveStateForException();
    void handleHardwareBreakpoints();
    INLINE bool handleSoftwareBreakpoints();
    bool executeInstructions(int count);

    void busError();

    struct Breakpoint {
        bool enabled = true;
        int hitCount = 0;
        bool hit = false;
        bool singleTime = false;

        Breakpoint() = default;
        Breakpoint(bool singleTime) : singleTime(singleTime) {}
    };
    std::unordered_map<uint32_t, Breakpoint> breakpoints;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(PC, nextPC, inBranchDelay, branchTaken);
        ar(slots);
        ar(reg, hi, lo);
        ar(cop0);
        ar(gte);
    }
};
};  // namespace mips
