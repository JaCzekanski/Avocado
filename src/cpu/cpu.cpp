#include "cpu.h"
#include <cassert>
#include "bios/functions.h"
#include "cpu/instructions.h"
#include "system.h"

namespace mips {
CPU::CPU(System* sys) : sys(sys), _opcode(0) {
    setPC(0xBFC00000);
    inBranchDelay = false;
    for (int i = 0; i < 32; i++) reg[i] = 0;
    hi = 0;
    lo = 0;
    exception = false;

    for (int i = 0; i < 2; i++) {
        slots[i].reg = 0;
    }
}

void CPU::loadDelaySlot(uint32_t r, uint32_t data) {
#ifdef ENABLE_LOAD_DELAY_SLOTS
    assert(r < REGISTER_COUNT);
    if (r == 0) return;
    if (r == slots[0].reg) slots[0].reg = 0;  // Override previous write to same register

    slots[1].reg = r;
    slots[1].data = data;
    slots[1].prevData = reg[r];
#else
    reg[r] = data;
#endif
}

void CPU::moveLoadDelaySlots() {
#ifdef ENABLE_LOAD_DELAY_SLOTS
    if (slots[0].reg != 0) {
        assert(slots[0].reg < REGISTER_COUNT);

        // If register contents has been changed during delay slot - ignore it
        if (reg[slots[0].reg] == slots[0].prevData) {
            reg[slots[0].reg] = slots[0].data;
        }
    }

    slots[0] = slots[1];
    slots[1].reg = 0;  // cancel
#endif
}

void CPU::saveStateForException() {
    exceptionPC = PC;
    exceptionIsInBranchDelay = inBranchDelay;
    exceptionIsBranchTaken = branchTaken;

    inBranchDelay = false;
    branchTaken = false;
}

bool CPU::executeInstructions(int count) {
    for (int i = 0; i < count; i++) {
        reg[0] = 0;

        // HACK: BIOS hooks
        uint32_t maskedPc = PC & 0x1FFFFF;
        if (maskedPc == 0xa0 || maskedPc == 0xb0 || maskedPc == 0xc0) sys->handleBiosFunction();

        saveStateForException();

        checkForInterrupts();

        // HACK: Following code does NOT follow specification, it is only used for fastboot hack.
        if (cop0.dcic.breakOnCode && PC == cop0.bpc) {
            cop0.dcic.codeBreakpointHit = 1;
            cop0.dcic.breakpointHit = 1;
            cop0.dcic.breakOnCode = 0;
            sys->state = System::State::pause;
            return false;
        }
        if (!breakpoints.empty()) {
            auto bp = breakpoints.find(PC);
            if (bp != breakpoints.end() && bp->second.enabled) {
                if (bp->second.singleTime) {
                    breakpoints.erase(bp);
                    sys->state = System::State::pause;
                    return false;
                }
                if (!bp->second.hit) {
                    bp->second.hitCount++;
                    bp->second.hit = true;
                    sys->state = System::State::pause;

                    return false;
                }
                bp->second.hit = false;
            }
        }

        _opcode = Opcode(sys->readMemory32(PC));
        const auto& op = instructions::OpcodeTable[_opcode.op];

        setPC(nextPC);

        op.instruction(this, _opcode);

        moveLoadDelaySlots();

        if (exception) {
            exception = false;
            return true;
        }

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
