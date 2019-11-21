#include "cpu.h"
#include "bios/functions.h"
#include "cpu/instructions.h"
#include "system.h"

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
    if (cop0.dcic.codeBreakpointEnabled()) {
        if (((PC ^ cop0.bpcm) & cop0.bpc) == 0) {
            cop0.dcic.codeBreakpointHit = 1;
            cop0.dcic.breakpointHit = 1;
            instructions::exception(this, COP0::CAUSE::Exception::breakpoint);
        }
    }
}

INLINE bool CPU::handleSoftwareBreakpoints() {
    if (breakpoints.empty()) {
        return false;
    }

    auto bp = breakpoints.find(PC);
    if (bp != breakpoints.end() && bp->second.enabled) {
        if (bp->second.singleTime) {
            breakpoints.erase(bp);
            sys->state = System::State::pause;
            return true;
        }
        if (!bp->second.hit) {
            bp->second.hitCount++;
            bp->second.hit = true;
            sys->state = System::State::pause;

            return true;
        }
        bp->second.hit = false;
    }
    return false;
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
        handleHardwareBreakpoints();

        if (unlikely(handleSoftwareBreakpoints())) return false;

        _opcode = Opcode(fetchInstruction(PC));
        const auto& op = instructions::OpcodeTable[_opcode.op];

        setPC(nextPC);

        op.instruction(this, _opcode);

        moveLoadDelaySlots();

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
