#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cassert>
#include "mips.h"
#include "mipsInstructions.h"
#include "psxExe.h"
#include "utils/file.h"
#include "bios/functions.h"
#include "device/spu.h"

namespace mips {
CPU::CPU() {
    PC = 0xBFC00000;
    jumpPC = 0;
    shouldJump = false;
    for (int i = 0; i < 32; i++) reg[i] = 0;
    hi = 0;
    lo = 0;
    exception = false;

    memset(bios, 0, BIOS_SIZE);
    memset(ram, 0, RAM_SIZE);
    memset(scratchpad, 0, SCRATCHPAD_SIZE);
    memset(expansion, 0, EXPANSION_SIZE);

    for (int i = 0; i < 2; i++) {
        slots[i] = {0};
    }

    memoryControl = new Dummy("MemCtrl", 0x1f801000, false);
    controller = new controller::Controller();
    controller->setCPU(this);
    serial = new Dummy("Serial", 0x1f801050, false);

    interrupt = new interrupt::Interrupt();
    interrupt->setCPU(this);

    dma = new dma::DMA();
    dma->setCPU(this);

    timer0 = new timer::Timer(0);
    timer0->setCPU(this);

    timer1 = new timer::Timer(1);
    timer1->setCPU(this);

    timer2 = new timer::Timer(2);
    timer2->setCPU(this);

    cdrom = new cdrom::CDROM();
    cdrom->setCPU(this);

    spu = new spu::SPU();
    spu->setCPU(this);

    mdec = new Dummy("MDEC", 0x1f801820);
    expansion2 = new Dummy("Expansion2", 0x1f802000, false);
}

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
        if (address >= 0x810 && address < 0x818) {
            static Reg32 d;
            if (((address - 0x810) & 3) == 0) d._reg = gpu->read(address - 0x810);

            return d.read((address - 0x810) & 3);
        }
        IO(0x820, 0x828, mdec);
        IO(0xC00, 0x1000, spu);
        if (address >= 0x1000 && address < 0x1043) {
            if (address == 0x1021) return 0x0c;
            return expansion2->read(address);
        }
        printf("R Unhandled IO at 0x%08x\n", address + 0x1f801000);
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
    if (address >= 0x1f000000 && address < 0x1f000000 + EXPANSION_SIZE) {
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
        if (address >= 0x810 && address < 0x818) {
            static Reg32 d;
            d.write((address - 0x810) & 3, data);
            if (((address - 0x810) & 3) == 3) gpu->write(address - 0x810, d._reg);
            return;
        }
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

void CPU::printFunctionInfo(int type, uint8_t number, bios::Function f) {
    printf("  BIOS %02X(%02x): %s(", type, number, f.name);
    for (int i = 0; i < f.argc; i++) {
        if (i > 4) break;
        printf("0x%x%s", reg[4 + i], i == (f.argc - 1) ? "" : ", ");
    }
    printf(")\n");
}

void CPU::handleBiosFunction() {
    std::unordered_map<uint8_t, bios::Function>::const_iterator function;
    uint32_t maskedPC = PC & 0x1FFFFF;
    uint8_t functionNumber = reg[9];
    bool log = biosLog;

    if (maskedPC == 0xA0) {
        function = bios::A0.find(functionNumber);
        if (function == bios::A0.end()) return;
        if (function->second.callback != nullptr) log = function->second.callback(*this);
    } else if (maskedPC == 0xB0) {
        function = bios::B0.find(functionNumber);
        if (function == bios::B0.end()) return;
        if (function->second.callback != nullptr) log = function->second.callback(*this);
    } else {
        if (log)
            printf("  BIOS %02X(%02x) (0x%02x, 0x%02x, 0x%02x, 0x%02x)\n", maskedPC >> 4, functionNumber, reg[4], reg[5], reg[6], reg[7]);
        return;
    }

    if (log) printFunctionInfo(maskedPC >> 4, function->first, function->second);
}

void CPU::loadDelaySlot(int r, uint32_t data) {
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
    mipsInstructions::Opcode _opcode;

    checkForInterrupts();
    for (int i = 0; i < count; i++) {
        reg[0] = 0;

        _opcode.opcode = readMemory32(PC);

        bool isJumpCycle = shouldJump;
        const auto &op = mipsInstructions::OpcodeTable[_opcode.op];
        _mnemonic = (char *)op.mnemnic;

        op.instruction(this, _opcode);

        moveLoadDelaySlots();

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
    }
}

void CPU::emulateFrame() {
    int systemCycles = 300;
    for (;;) {
        if (!executeInstructions(systemCycles / 3)) {
            printf("CPU Halted\n");
            return;
        }

        dma->step();
        cdrom->step();
        timer1->step(systemCycles);
        timer2->step(systemCycles);

        if (gpu->emulateGpuCycles(systemCycles)) {
            interrupt->IRQ(interrupt::VBLANK);
            return;  // frame emulated
        }
    }
}

bool CPU::loadExeFile(std::string exePath) {
    auto _exe = getFileContents(exePath);
    PsxExe exe;
    if (_exe.empty()) return false;
    assert(_exe.size() >= 0x800);

    memcpy(&exe, &_exe[0], sizeof(exe));

    if (exe.t_size > _exe.size() - 0x800) {
        printf("Invalid exe t_size: 0x%08x\n", exe.t_size);
        return false;
    }

    for (size_t i = 0; i < exe.t_size; i++) {
        writeMemory8(exe.t_addr + i, _exe[0x800 + i]);
    }

    PC = exe.pc0;
    //    reg[28] = exe.gp0;
    //    reg[29] = exe.s_addr + exe.s_size;
    //    reg[30] = exe.s_addr + exe.s_size;

    exception = false;
    shouldJump = false;
    jumpPC = 0;

    // Reset timers
    writeMemory16(0x1f801104, 0x1c00);
    writeMemory16(0x1f801114, 0x1c00);
    writeMemory16(0x1f801124, 0x1c00);

    return true;
}

bool CPU::loadBios(std::string name) {
    std::string path = "data/bios/";
    auto _bios = getFileContents(path + name);
    if (_bios.empty()) {
        printf("Cannot open BIOS");
        return false;
    }
    assert(_bios.size() == 512 * 1024);
    std::copy(_bios.begin(), _bios.end(), bios);
    state = mips::CPU::State::run;
    return true;
}

bool CPU::loadExpansion(std::string name) {
    std::string path = "data/bios/";
    auto _exp = getFileContents(path + name);
    if (_exp.empty()) {
        return false;
    }
    assert(_exp.size() < EXPANSION_SIZE);
    std::copy(_exp.begin(), _exp.end(), expansion);
    return true;
}

void CPU::dumpRam() {
    std::vector<uint8_t> ram;
    ram.assign(this->ram, this->ram + 0x1fffff);
    putFileContents("ram.bin", ram);
}
}
