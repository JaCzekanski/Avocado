#include "mips.h"
#include "mipsInstructions.h"
#include <cstdio>
#include "psxExe.h"
#include "utils/file.h"
#include <cstdlib>
#include "bios/functions.h"

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
    if (address >= 0x1f000000 && address < 0x1f800000) return expansion[address - 0x1f000000];
    if (address >= 0x1f800000 && address < 0x1f800400) return scratchpad[address - 0x1f800000];
    if (address >= 0x1fc00000 && address < 0x1fc80000) return bios[address - 0x1fc00000];

#define IO(begin, end, periph)                   \
    if (address >= (begin) && address < (end)) { \
        return periph->read(address - (begin));  \
    }

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
        printf("R Unhandled IO at 0x%08x\n", address + 0x1f801000);
        __debugbreak();
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

uint8_t CPU::readMemory8(uint32_t address) { return readMemory(address); }

#define READ16(x, addr) (x[addr] | (x[addr + 1] << 8))

uint16_t CPU::readMemory16(uint32_t address) {
    uint32_t addr = address & 0x1FFFFFFF;
    if (address < 0x200000 * 4) {
        addr &= 0x1FFFFF;
        return READ16(ram, addr);
    } else if (address >= 0x1fc00000 && address < 0x1fc80000) {
        addr -= 0x1fc00000;
        return READ16(bios, addr);
    } else {
        uint16_t data = 0;
        data |= readMemory(address + 0);
        data |= readMemory(address + 1) << 8;
        return data;
    }
}

#undef READ16

#define READ32(x, addr) (x[addr] | (x[addr + 1] << 8) | (x[addr + 2] << 16) | (x[addr + 3] << 24))

uint32_t CPU::readMemory32(uint32_t address) {
    uint32_t addr = address & 0x1FFFFFFF;
    if (addr < 0x200000 * 4) {
        addr &= 0x1FFFFF;
        return READ32(ram, addr);
    } else if (addr >= 0x1fc00000 && addr < 0x1fc80000) {
        addr -= 0x1fc00000;
        return READ32(bios, addr);
    } else {
        uint32_t data = 0;
        data |= readMemory(address + 0);
        data |= readMemory(address + 1) << 8;
        data |= readMemory(address + 2) << 16;
        data |= readMemory(address + 3) << 24;
        return data;
    }
}

#undef READ32

void CPU::writeMemory8(uint32_t address, uint8_t data) { writeMemory(address, data); }

void CPU::writeMemory16(uint32_t address, uint16_t data) {
    writeMemory(address + 0, data & 0xff);
    writeMemory(address + 1, data >> 8);
}

void CPU::writeMemory32(uint32_t address, uint32_t data) {
    writeMemory(address + 0, data);
    writeMemory(address + 1, data >> 8);
    writeMemory(address + 2, data >> 16);
    writeMemory(address + 3, data >> 24);
}

void CPU::printFunctionInfo(int type, bios::Function f) {
    printf("  BIOS %02X(%02x): %s(", type, f.number, f.name);
    for (int i = 0; i < f.argc; i++) {
        if (i > 4) break;
        printf("0x%x%s", reg[4 + i], i == (f.argc - 1) ? "" : ", ");
    }
    printf(")\n");
}

void CPU::findFunctionInTable(const std::vector<bios::Function> &functions, int functionNumber) {
    for (auto function : functions) {
        if (function.number != functionNumber) {
            continue;
        }

        if (function.callback != nullptr) {
            bool log = function.callback(*this);
            if (!log) return;
        }

        if (biosLog) {
            printFunctionInfo(0xa0, function);
            return;
        }
    }
}

void CPU::handleBiosFunction() {
    uint32_t maskedPC = PC & 0x1FFFFF;
    int functionNumber = reg[9];

    if (maskedPC == 0xA0)
        findFunctionInTable(bios::A0, functionNumber);
    else if (maskedPC == 0xB0)
        findFunctionInTable(bios::B0, functionNumber);
    else {
        if (biosLog) printf("%x(0x%02x) (0x%02x, 0x%02x, 0x%02x, 0x%02x)\n", (PC & 0xf0) >> 4, reg[9], reg[4], reg[5], reg[6], reg[7]);
    }
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

            uint32_t maskedPc = PC & 0x1FFFFF;
            if (maskedPc == 0xa0 || maskedPc == 0xb0 || maskedPc == 0xc0) handleBiosFunction();
        } else {
            PC += 4;
        }
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

void CPU::dumpRam() {
    std::vector<uint8_t> ram(ram, ram + 0x200000 - 1);
    putFileContents("ram.bin", ram);
}
}
