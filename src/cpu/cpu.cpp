#include "cpu.h"
#include "bios/functions.h"
#include "cpu/instructions.h"
#include "system.h"


#include <iostream>
#include <string> 
#include <csignal>


namespace mips {
CPU::CPU(System* sys) : sys(sys), _opcode(0) {
    setPC(0xBFC00000);
    inBranchDelay = false;
    icacheEnabled = false;
    for (auto& r : reg) r = 0;
    hi = 0;
    lo = 0;

    for (auto& slot : slots) slot = {DUMMY_REG, 0};
    for (auto& line : icache) line = {0, 0};
}

INLINE void CPU::moveLoadDelaySlots() {
    reg[slots[0].reg] = slots[0].data;
    slots[0] = slots[1];
    slots[1].reg = DUMMY_REG;  // invalidate
}

void CPU::saveStateForException() {
    exceptionPC = PC;
    exceptionIsInBranchDelay = inBranchDelay;
    exceptionIsBranchTaken = branchTaken;

    inBranchDelay = false;
    branchTaken = false;
}

void CPU::handleHardwareBreakpoints() {
    if (cop0.dcic.codeBreakpointEnabled() && ((PC ^ cop0.bpcm) & cop0.bpc) == 0) {
        cop0.dcic.codeBreakpointHit = 1;
        cop0.dcic.breakpointHit = 1;
        instructions::exception(this, COP0::CAUSE::Exception::breakpoint);
    }
}

bool CPU::handleSoftwareBreakpoints() {
    if (breakpoints.empty()) return false;

    auto bp = breakpoints.find(PC);
    if (bp == breakpoints.end()) return false;
    if (!bp->second.enabled) return false;

    bp->second.hitCount++;
    if (bp->second.singleTime) {
        removeBreakpoint(PC);
    }
    sys->state = System::State::pause;
    return true;
}

// https://stackoverflow.com/questions/23502153/utf-8-encoding-algorithm-vs-utf-16-algorithm
void GetUnicodeCharUtf16(unsigned int code, unsigned short chars[2]){
    if ( ((code >= 0x0000) && (code <= 0xD7FF)) ||
        ((code >= 0xE000) && (code <= 0xFFFF)) )
    {
        chars[0] = 0x0000;
        chars[1] = (unsigned short) code;
    }
    else if ((code >= 0xD800) && (code <= 0xDFFF))
    {
        // unicode replacement character
        chars[0] = 0x0000;
        chars[1] = 0xFFFD;
    }
    else
    {
        // surrogate pair
        code -= 0x010000;
        chars[0] = 0xD800 + (unsigned short)((code >> 10) & 0x3FF);
        chars[1] = 0xDC00 + (unsigned short)(code & 0x3FF);
    }
}

// https://stackoverflow.com/questions/23461499/decimal-to-unicode-char-in-c
void GetUnicodeCharUtf8(unsigned int code, char chars[5]) {
    if (code <= 0x7F) {
        chars[0] = (code & 0x7F); chars[1] = '\0';
    } else if (code <= 0x7FF) {
        // one continuation byte
        chars[1] = 0x80 | (code & 0x3F); code = (code >> 6);
        chars[0] = 0xC0 | (code & 0x1F); chars[2] = '\0';
    } else if (code <= 0xFFFF) {
        // two continuation bytes
        chars[2] = 0x80 | (code & 0x3F); code = (code >> 6);
        chars[1] = 0x80 | (code & 0x3F); code = (code >> 6);
        chars[0] = 0xE0 | (code & 0xF); chars[3] = '\0';
    } else if (code <= 0x10FFFF) {
        // three continuation bytes
        chars[3] = 0x80 | (code & 0x3F); code = (code >> 6);
        chars[2] = 0x80 | (code & 0x3F); code = (code >> 6);
        chars[1] = 0x80 | (code & 0x3F); code = (code >> 6);
        chars[0] = 0xF0 | (code & 0x7); chars[4] = '\0';
    } else {
        // unicode replacement character
        chars[2] = 0xEF; chars[1] = 0xBF; chars[0] = 0xBD;
        chars[3] = '\0';
    }
}

INLINE uint32_t CPU::fetchInstruction(uint32_t address) {
    // Only KUSEG and KSEG0 have code-cache
    // I'm completely clueless if address comparison here makes any sense
    // Host CPU branch predictor sure isn't happy about it
    if (unlikely(!icacheEnabled) || address >= 0xa000'0000) {
        return sys->readMemory32(address);
    }

    uint32_t tag = ((address & 0xfffff000) >> 12) | CACHE_LINE_VALID_BIT;
    uint16_t index = (address & 0xffc) >> 2;

    CacheLine line = icache[index];
    if (line.tag == tag) {
        return line.data;
    }


    uint32_t data = sys->readMemory32(address);

    //char chars[2];
    //GetUnicodeCharUtf16(data, chars);
    //std::cout << chars << "\n";

    if (data >= 0x02CF6648 && data <= 0x09480000){
        //printf("%X\n", data);
        //printf("ADDR dec: %u hex: %X\n", address, address);
        printf("DATA dec: %u hex: 0x%X\n", data, data);
    }
    //printf("DATA dec: %u hex: %X\n", data, data);
    //printf("%X\n", data);
    icache[index] = CacheLine{tag, data};

    return data;
}

bool CPU::executeInstructions(int count) {
    for (int i = 0; i < count; i++) {
        // HACK: BIOS hooks
        uint32_t maskedPc = PC & 0x1fff'ffff;
        if (maskedPc == 0xa0 || maskedPc == 0xb0 || maskedPc == 0xc0) sys->handleBiosFunction();

        saveStateForException();
        checkForInterrupts();
        if (unlikely(breakpointsEnabled)) {
            handleHardwareBreakpoints();
            if (handleSoftwareBreakpoints()) return false;
        }

        _opcode = Opcode(fetchInstruction(PC));

        const auto& op = instructions::OpcodeTable[_opcode.op];

        setPC(nextPC);

        op.instruction(this, _opcode);

        moveLoadDelaySlots();

        sys->cycles++;
        if (sys->state != System::State::run) return false;
    }
    return true;
}

void CPU::checkForInterrupts() {
    if ((cop0.cause.interruptPending & cop0.status.interruptMask) && cop0.status.interruptEnable) {
        instructions::exception(this, COP0::CAUSE::Exception::interrupt);
    }
}

void CPU::busError() { instructions::exception(this, COP0::CAUSE::Exception::busErrorData); }

}  // namespace mips
