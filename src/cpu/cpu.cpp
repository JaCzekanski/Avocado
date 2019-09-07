#include "cpu.h"
#include <cassert>
#include "bios/functions.h"
#include "cpu/instructions.h"
#include "system.h"

namespace mips {
CPU::CPU(System* sys) : sys(sys), _opcode(0) {
    setPC(0xBFC00000);
    inBranchDelay = false;
    for (auto& r : reg) r = 0;
    hi = 0;
    lo = 0;

    for (auto& slot : slots) slot.reg = 0;
}

void CPU::loadDelaySlot(uint32_t r, uint32_t data) {
    assert(r < REGISTER_COUNT);
    if (r == 0) return;
    if (r == slots[0].reg) slots[0].reg = 0;  // Override previous write to same register

    slots[1].reg = r;
    slots[1].data = data;
    slots[1].prevData = reg[r];
}

void CPU::moveLoadDelaySlots() {
    if (slots[0].reg != 0) {
        assert(slots[0].reg < REGISTER_COUNT);

        // If register contents has been changed during delay slot - ignore it
        if (reg[slots[0].reg] == slots[0].prevData) {
            reg[slots[0].reg] = slots[0].data;
        }
    }

    slots[0] = slots[1];
    slots[1].reg = 0;  // cancel
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

bool CPU::handleSoftwareBreakpoints() {
    if (!breakpoints.empty()) {
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
    }
    return false;
}

bool CPU::executeInstructions(int count) {
    for (int i = 0; i < count; i++) {
        // HACK: BIOS hooks
        uint32_t maskedPc = PC & 0x1FFFFF;
        if (maskedPc == 0xa0 || maskedPc == 0xb0 || maskedPc == 0xc0) sys->handleBiosFunction();

        saveStateForException();
        checkForInterrupts();
        handleHardwareBreakpoints();

        if (handleSoftwareBreakpoints()) return false;

        _opcode = Opcode(sys->readMemory32(PC));
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
}  // namespace mips
