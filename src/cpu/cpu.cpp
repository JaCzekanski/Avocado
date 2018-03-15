#include "cpu.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "bios/functions.h"
#include "cpu/instructions.h"
#include "system.h"

namespace mips {
CPU::CPU(System* sys) : sys(sys) {
    PC = 0xBFC00000;
    jumpPC = 0;
    shouldJump = false;
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

bool CPU::executeInstructions(int count) {
    checkForInterrupts();
    for (int i = 0; i < count; i++) {
        reg[0] = 0;

#ifdef ENABLE_BREAKPOINTS
        if (cop0.dcic & (1 << 24) && PC == cop0.bpc) {
            cop0.dcic &= ~(1 << 24);  // disable breakpoint
            state = State::pause;
            return false;
        }
#endif
        if (!breakpoints.empty()) {
            auto bp = breakpoints.find(PC);
            if (bp != breakpoints.end() && bp->second.enabled) {
                if (!bp->second.hit) {
                    bp->second.hitCount++;
                    bp->second.hit = true;
                    sys->state = System::State::pause;

                    return false;
                }
                bp->second.hit = false;
            }
        }

        Opcode _opcode(sys->readMemory32(PC));

        bool isJumpCycle = shouldJump;
        const auto& op = instructions::OpcodeTable[_opcode.op];

        op.instruction(this, _opcode);

        moveLoadDelaySlots();

        if (exception) {
            exception = false;
            return true;
        }

        if (sys->state != System::State::run) return false;
        if (isJumpCycle) {
            PC = jumpPC & 0xFFFFFFFC;
            jumpPC = 0;
            shouldJump = false;

            uint32_t maskedPc = PC & 0x1FFFFF;
            if (maskedPc == 0xa0 || maskedPc == 0xb0 || maskedPc == 0xc0) sys->handleBiosFunction();
        } else {
            PC += 4;
        }
    }
    return true;
}

void CPU::checkForInterrupts() {
    if ((cop0.cause.interruptPending & 4) && cop0.status.interruptEnable && (cop0.status.interruptMask & 4)) {
        instructions::exception(this, COP0::CAUSE::Exception::interrupt);
    }
}
}  // namespace mips
