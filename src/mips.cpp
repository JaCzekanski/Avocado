#include "mips.h"
#include "mipsInstructions.h"
#include <cstdio>
#include <string>
#include <deque>
#include "psxExe.h"
#include "utils/file.h"
#include <cstdlib>
#include <cstring>

extern bool disassemblyEnabled;
extern char *_mnemonic;
extern std::string _disasm;

extern uint32_t htimer;

namespace mips {
const char *regNames[] = {"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
                          "s0",   "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"};

uint8_t CPU::readMemory(uint32_t address) {
    // if (address >= 0xfffe0130 && address < 0xfffe0134) {
    //    printf("R Unhandled memory control\n");
    //    return 0;
    //}
    address &= 0x1FFFFFFF;

    if (address < 0x200000 * 4) return ram[address & 0x1FFFFF];
    if (address >= 0x1f000000 && address < 0x1f800000) {
        return expansion[address - 0x1f000000];
    }
    if (address >= 0x1f800000 && address < 0x1f800400) return scratchpad[address - 0x1f800000];

    if (address >= 0x1fc00000 && address < 0x1fc80000) return bios[address - 0x1fc00000];
#define IO(begin, end, periph) \
    if (address >= (begin) && address < (end)) return periph->read(address - (begin))

    // IO Ports
    if (address >= 0x1f801000 && address <= 0x1f803000) {
        address -= 0x1f801000;

        if (address >= 0x50 && address < 0x60) {
            if (address >= 0x54 && address < 0x58) {
                return 0x5 >> ((address - 0x54) * 8);
            }
        }
        if (address >= 0xdaa && address < 0xdad) {
            return rand();
        }

        IO(0x00, 0x24, memoryControl);
        IO(0x40, 0x50, controller);
        IO(0x50, 0x60, serial);
        IO(0x60, 0x64, memoryControl);
        IO(0x70, 0x78, interrupt);
        IO(0x80, 0x100, dma);
        IO(0x100, 0x110, timer0);
        IO(0x110, 0x120, timer1);
        IO(0x120, 0x130, timer2);
        IO(0x800, 0x804, cdrom);
        IO(0x810, 0x818, gpu);
        IO(0x820, 0x828, mdec);
        IO(0xC00, 0x1000, spu);
        if (address >= 0x1000 && address < 0x1043) {
            if (address == 0x1021) return 0x0c;
            return expansion2->read(address);
        }
        printf("R Unhandled IO at 0x%08x\n", address);
    }
#undef IO

    // printf("R Unhandled address at 0x%08x\n", address);
    return 0;
}

void CPU::writeMemory(uint32_t address, uint8_t data) {
    // Cache control
    // if (address >= 0xfffe0130 && address < 0xfffe0134) {
    //    printf("W Unhandled memory control\n");
    //    return;
    //}
    address &= 0x1FFFFFFF;

    if (address < 0x200000 * 4) {
        if (!cop0.status.isolateCache) ram[address & 0x1FFFFF] = data;
        return;
    }
    if (address >= 0x1f000000 && address < 0x1f010000) {
        expansion[address - 0x1f000000] = data;
        return;
    }
    if (address >= 0x1f800000 && address < 0x1f800400) {
        scratchpad[address - 0x1f800000] = data;
        return;
    }

#define IO(begin, end, periph) \
    if (address >= (begin) && address < (end)) return periph->write(address - (begin), data)

    // IO Ports
    if (address >= 0x1f801000 && address <= 0x1f803000) {
        address -= 0x1f801000;

        if (address == 0x1023) printf("%c", data);  // Debug

        IO(0x00, 0x24, memoryControl);
        IO(0x40, 0x50, controller);
        IO(0x50, 0x60, serial);
        IO(0x60, 0x64, memoryControl);
        IO(0x70, 0x78, interrupt);
        IO(0x80, 0x100, dma);
        IO(0x100, 0x110, timer0);
        IO(0x110, 0x120, timer1);
        IO(0x120, 0x130, timer2);
        IO(0x800, 0x804, cdrom);
        IO(0x810, 0x818, gpu);
        IO(0x820, 0x828, mdec);
        IO(0xC00, 0x1000, spu);
        IO(0x1000, 0x1043, expansion2);
        printf("W Unhandled IO at 0x%08x: 0x%02x\n", address, data);
        return;
    }
#undef IO
    // printf("W Unhandled address at 0x%08x: 0x%02x\n", address, data);
}

uint8_t CPU::readMemory8(uint32_t address) {
    uint8_t data = readMemory(address);
    if (memoryAccessLogging) printf("R8:  0x%08x - 0x%02x\n", address, data);
    return data;
}

uint16_t CPU::readMemory16(uint32_t address) {
    uint16_t data = 0;
    data |= readMemory(address + 0);
    data |= readMemory(address + 1) << 8;
    if (memoryAccessLogging) printf("R16: 0x%08x - 0x%04x\n", address, data);
    return data;
}

uint32_t CPU::readMemory32(uint32_t address) {
    uint32_t data = 0;
    data |= readMemory(address + 0);
    data |= readMemory(address + 1) << 8;
    data |= readMemory(address + 2) << 16;
    data |= readMemory(address + 3) << 24;
    if (memoryAccessLogging) printf("R32: 0x%08x - 0x%08x\n", address, data);
    return data;
}

void CPU::writeMemory8(uint32_t address, uint8_t data) {
    writeMemory(address, data);
    if (memoryAccessLogging) printf("W8:  0x%08x - 0x%02x\n", address, data);
}

void CPU::writeMemory16(uint32_t address, uint16_t data) {
    writeMemory(address + 0, data & 0xff);
    writeMemory(address + 1, data >> 8);
    if (memoryAccessLogging) printf("W16: 0x%08x - 0x%04x\n", address, data);
}

void CPU::writeMemory32(uint32_t address, uint32_t data) {
    writeMemory(address + 0, data);
    writeMemory(address + 1, data >> 8);
    writeMemory(address + 2, data >> 16);
    writeMemory(address + 3, data >> 24);
    if (memoryAccessLogging) printf("W32: 0x%08x - 0x%08x\n", address, data);
}

bool CPU::executeInstructions(int count) {
    mipsInstructions::Opcode _opcode;

    checkForInterrupts();
    for (int i = 0; i < count; i++) {
        reg[0] = 0;
        _opcode.opcode = readMemory32(PC);

        bool isJumpCycle = shouldJump;
        const auto &op = mipsInstructions::OpcodeTable[_opcode.op];
        _mnemonic = op.mnemnic;

        if ((PC & 0x0fffffff) == 0xa0 && biosLog) {
            printf("BIOS A(0x%02x) r4: 0x%08x r5: 0x%08x r6: 0x%08x \n", reg[9], reg[4], reg[5], reg[6]);
        }
        if ((PC & 0x0fffffff) == 0xb0 && (biosLog || reg[9] == 0x3d)) {
            if (reg[9] == 0x3d) {
                printf("%c", reg[4]);
            } else {
                printf("BIOS B(0x%02x)\n", reg[9]);
            }
        }
        if ((PC & 0x0fffffff) == 0xc0 && biosLog) {
            printf("BIOS C(0x%02x)\n", reg[9]);
        }
        op.instruction(this, _opcode);

        if (disassemblyEnabled) {
            printf("   0x%08x  %08x:    %s %s\n", PC, _opcode.opcode, _mnemonic, _disasm.c_str());
        }

        if (exception) {
            exception = false;
            return true;
        }

        if (state != State::run) return false;
        if (isJumpCycle) {
            PC = jumpPC & 0xFFFFFFFC;
            jumpPC = 0;
            shouldJump = false;
        } else
            PC += 4;
    }
    return true;
}

void CPU::checkForInterrupts() {
    if ((cop0.cause.interruptPending & 4) && cop0.status.interruptEnable && (cop0.status.interruptMask & 4)) {
        mipsInstructions::exception(this, cop0::CAUSE::Exception::interrupt);
        // printf("-%s\n", interrupt->getStatus().c_str());
    }
}

bool CPU::loadExeFile(std::string exePath) {
    auto _exe = getFileContents(exePath);
    PsxExe exe;
    if (_exe.empty()) return false;

    memcpy(&exe, &_exe[0], sizeof(exe));

    for (size_t i = 0x800; i < _exe.size(); i++) {
        writeMemory8(exe.t_addr + i - 0x800, _exe[i]);
    }

    PC = exe.pc0;
    shouldJump = false;
    jumpPC = 0;

    for (int i = 0; i < 32; i++) reg[i] = 0;
    hi = 0;
    lo = 0;
    reg[29] = exe.s_addr;
    return false;
}
}
