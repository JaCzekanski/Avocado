#include "mips.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "bios/functions.h"
#include "cpu/instructions.h"
#include "utils/file.h"
#include "utils/psx_exe.h"

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
        slots[i].reg = 0;
    }

    memoryControl = std::make_unique<Dummy>("MemCtrl", 0x1f801000, false);
    controller = std::make_unique<controller::Controller>(this);
    serial = std::make_unique<Dummy>("Serial", 0x1f801050, false);
    interrupt = std::make_unique<Interrupt>(this);
    gpu = std::make_unique<GPU>();
    dma = std::make_unique<dma::DMA>(this);
    timer0 = std::make_unique<Timer<0>>(this);
    timer1 = std::make_unique<Timer<1>>(this);
    timer2 = std::make_unique<Timer<2>>(this);
    cdrom = std::make_unique<cdrom::CDROM>(this);
    spu = std::make_unique<SPU>();
    mdec = std::make_unique<MDEC>();
    expansion2 = std::make_unique<Dummy>("Expansion2", 0x1f802000, false);
}

// Note: stupid static_casts and asserts are only to supress MSVC warnings

// Warning: This function does not check array boundaries. Make sure that address is aligned!
template <typename T>
INLINE T read_fast(uint8_t* device, uint32_t addr) {
    static_assert(std::is_same<T, uint8_t>() || std::is_same<T, uint16_t>() || std::is_same<T, uint32_t>(), "Invalid type used");

    if (sizeof(T) == 1) return static_cast<T>(((uint8_t*)device)[addr]);
    if (sizeof(T) == 2) return static_cast<T>(((uint16_t*)device)[addr / 2]);
    if (sizeof(T) == 4) return static_cast<T>(((uint32_t*)device)[addr / 4]);
    return 0;
}

// Warning: This function does not check array boundaries. Make sure that address is aligned!
template <typename T>
INLINE void write_fast(uint8_t* device, uint32_t addr, T value) {
    static_assert(std::is_same<T, uint8_t>() || std::is_same<T, uint16_t>() || std::is_same<T, uint32_t>(), "Invalid type used");

    if (sizeof(T) == 1)
        ((uint8_t*)device)[addr] = static_cast<uint8_t>(value);
    else if (sizeof(T) == 2)
        ((uint16_t*)device)[addr / 2] = static_cast<uint16_t>(value);
    else if (sizeof(T) == 4)
        ((uint32_t*)device)[addr / 4] = static_cast<uint32_t>(value);
}

template <typename T, typename Device>
INLINE T read_io(Device& periph, uint32_t addr) {
    static_assert(std::is_same<T, uint8_t>() || std::is_same<T, uint16_t>() || std::is_same<T, uint32_t>(), "Invalid type used");

    if (sizeof(T) == 1) return periph->read(addr);
    if (sizeof(T) == 2) return periph->read(addr) | periph->read(addr + 1) << 8;
    if (sizeof(T) == 4)
        return periph->read(addr) | periph->read(addr + 1) << 8 | periph->read(addr + 2) << 16 | periph->read(addr + 3) << 24;
    return 0;
}

template <typename T, typename Device>
INLINE void write_io(Device& periph, uint32_t addr, T data) {
    static_assert(std::is_same<T, uint8_t>() || std::is_same<T, uint16_t>() || std::is_same<T, uint32_t>(), "Invalid type used");

    if (sizeof(T) == 1) {
        periph->write(addr, (static_cast<uint8_t>(data)) & 0xff);
    } else if (sizeof(T) == 2) {
        periph->write(addr, (static_cast<uint16_t>(data)) & 0xff);
        periph->write(addr + 1, (static_cast<uint16_t>(data) >> 8) & 0xff);
    } else if (sizeof(T) == 4) {
        periph->write(addr, (static_cast<uint32_t>(data)) & 0xff);
        periph->write(addr + 1, (static_cast<uint32_t>(data) >> 8) & 0xff);
        periph->write(addr + 2, (static_cast<uint32_t>(data) >> 16) & 0xff);
        periph->write(addr + 3, (static_cast<uint32_t>(data) >> 24) & 0xff);
    }
}

template <typename T>
INLINE T CPU::readMemory(uint32_t address) {
    static_assert(std::is_same<T, uint8_t>() || std::is_same<T, uint16_t>() || std::is_same<T, uint32_t>(), "Invalid type used");

    uint32_t addr = address;

    // align address
    if (sizeof(T) == 1)
        addr &= 0x1fffffff;
    else if (sizeof(T) == 2)
        addr &= 0x1ffffffe;
    else if (sizeof(T) == 4)
        addr &= 0x1ffffffc;

    if (addr < 0x200000 * 4) return read_fast<T>(ram, addr & 0x1fffff);
    if (addr >= 0x1f000000 && addr < 0x1f000000 + EXPANSION_SIZE) return read_fast<T>(expansion, addr - 0x1f000000);
    if (addr >= 0x1f800000 && addr < 0x1f800400) return read_fast<T>(scratchpad, addr - 0x1f800000);
    if (addr >= 0x1fc00000 && addr < 0x1fc80000) return read_fast<T>(bios, addr - 0x1fc00000);

#define IO(begin, end, periph) \
    if (addr >= (begin) && addr < (end)) return read_io<T>((periph), addr - (begin))

    // IO Ports
    if (addr >= 0x1f801000 && addr <= 0x1f803000) {
        addr -= 0x1f801000;

        if (addr >= 0x50 && addr < 0x60) {
            if (addr >= 0x54 && addr < 0x58) {
                return 0x5 >> ((addr - 0x54) * 8);
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
        //        IO(0x810, 0x0818, gpu);
        if (addr >= (0x810) && addr < (0x0818)) {
            if (sizeof(T) == 4)
                return gpu->read(addr - 0x810);
            else
                printf("W Unsupported access to GPU with bit size: %d\n", static_cast<int>(sizeof(T) * 8));
        }

        IO(0x820, 0x828, mdec);
        IO(0xC00, 0x1000, spu);
        IO(0x1000, 0x1043, expansion2);
        printf("R Unhandled IO at 0x%08x\n", addr + 0x1f801000);
    }
#undef IO

    if (address >= 0xfffe0130 && address < 0xfffe0134) {
        printf("R Unhandled memory control\n");
        return 0;
    }

    printf("R Unhandled address at 0x%08x\n", address);
    return 0;
}
template <typename T>
INLINE void CPU::writeMemory(uint32_t address, T data) {
    static_assert(std::is_same<T, uint8_t>() || std::is_same<T, uint16_t>() || std::is_same<T, uint32_t>(), "Invalid type used");

    uint32_t addr = address;

    // align address
    if (sizeof(T) == 1)
        addr &= 0x1fffffff;
    else if (sizeof(T) == 2)
        addr &= 0x1ffffffe;
    else if (sizeof(T) == 4)
        addr &= 0x1ffffffc;

    if (addr < 0x200000 * 4) {
        if (cop0.status.isolateCache) return;
        return write_fast<T>(ram, addr & 0x1fffff, data);
    }
    if (addr >= 0x1f000000 && addr < 0x1f000000 + EXPANSION_SIZE) {
        return write_fast<T>(expansion, addr - 0x1f000000, data);
    }
    if (addr >= 0x1f800000 && addr < 0x1f800400) {
        return write_fast<T>(scratchpad, addr - 0x1f800000, data);
    }

#define IO(begin, end, periph) \
    if (addr >= (begin) && addr < (end)) return write_io<T>((periph), addr - (begin), data);

    // IO Ports
    if (addr >= 0x1f801000 && addr <= 0x1f803000) {
        addr -= 0x1f801000;

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
        if (addr >= (0x810) && addr < (0x0818)) {
            if (sizeof(T) == 4)
                return gpu->write(addr - 0x810, data);
            else
                printf("W Unsupported access to GPU with bit size: %d\n", static_cast<int>(sizeof(T) * 8));
        }
        IO(0x820, 0x828, mdec);
        IO(0xC00, 0x1000, spu);
        IO(0x1000, 0x1043, expansion2);
        printf("W Unhandled IO at 0x%08x: 0x%02x\n", addr, data);
    }
#undef IO

    // Cache control
    if (address >= 0xfffe0130 && address < 0xfffe0134) {
        printf("W Unhandled memory control\n");
        return;
    }

    printf("W Unhandled address at 0x%08x: 0x%02x\n", address, data);
}

#ifdef ENABLE_IO_LOG
#define LOG_IO(mode, size, addr, data)                             \
    do {                                                           \
        if ((addr) >= 0x1f801000 && (addr) <= 0x1f803000) {        \
            ioLogList.push_back({(mode), (size), (addr), (data)}); \
        }                                                          \
    } while (0)
#else
#define LOG_IO(mode, size, addr, data) \
    do {                               \
    } while (0)
#endif

uint8_t CPU::readMemory8(uint32_t address) {
    uint8_t data = readMemory<uint8_t>(address);
    LOG_IO(IO_LOG_ENTRY::MODE::READ, 8, address, data);
    return data;
}

uint16_t CPU::readMemory16(uint32_t address) {
    uint16_t data = readMemory<uint16_t>(address);
    LOG_IO(IO_LOG_ENTRY::MODE::READ, 16, address, data);
    return data;
}

uint32_t CPU::readMemory32(uint32_t address) {
    uint32_t data = readMemory<uint32_t>(address);
    LOG_IO(IO_LOG_ENTRY::MODE::READ, 32, address, data);
    return data;
}

void CPU::writeMemory8(uint32_t address, uint8_t data) {
    LOG_IO(IO_LOG_ENTRY::MODE::WRITE, 8, address, data);
    writeMemory<uint8_t>(address, data);
}

void CPU::writeMemory16(uint32_t address, uint16_t data) {
    LOG_IO(IO_LOG_ENTRY::MODE::WRITE, 16, address, data);
    writeMemory<uint16_t>(address, data);
}

void CPU::writeMemory32(uint32_t address, uint32_t data) {
    LOG_IO(IO_LOG_ENTRY::MODE::WRITE, 32, address, data);
    writeMemory<uint32_t>(address, data);
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
                    state = State::pause;

                    return false;
                }
                bp->second.hit = false;
            }
        }

        Opcode _opcode(readMemory32(PC));

        bool isJumpCycle = shouldJump;
        const auto& op = instructions::OpcodeTable[_opcode.op];

        op.instruction(this, _opcode);

        moveLoadDelaySlots();

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
        instructions::exception(this, COP0::CAUSE::Exception::interrupt);
    }
}

void CPU::singleStep() {
    state = State::run;
    executeInstructions(1);
    state = State::pause;

    dma->step();
    cdrom->step();
    timer0->step(3);
    timer1->step(3);
    timer2->step(3);
    controller->step();

    if (gpu->emulateGpuCycles(3)) {
        interrupt->trigger(interrupt::VBLANK);
    }
}

void CPU::emulateFrame() {
#ifdef ENABLE_IO_LOG
    ioLogList.clear();
#endif
    gte.log.clear();
    gpu->gpuLogList.clear();

    gpu->prevVram = gpu->vram;
    int systemCycles = 300;
    for (;;) {
        if (!executeInstructions(systemCycles / 3)) {
            // printf("CPU Halted\n");
            return;
        }

        dma->step();
        cdrom->step();
        timer0->step(systemCycles);
        timer1->step(systemCycles);
        timer2->step(systemCycles);

        static int wtf = 0;
        if (++wtf % 200 == 0) interrupt->trigger(interrupt::TIMER2);
        controller->step();

        if (gpu->emulateGpuCycles(systemCycles)) {
            interrupt->trigger(interrupt::VBLANK);
            return;  // frame emulated
        }
    }
}

void CPU::softReset() {
    printf("Soft reset\n");
    PC = 0xBFC00000;
    shouldJump = false;
    state = State::run;
}

bool CPU::loadExeFile(std::string exePath) {
    auto _exe = getFileContents(exePath);
    PsxExe exe;
    if (_exe.empty()) return false;
    assert(_exe.size() >= 0x800);

    memcpy(&exe, _exe.data(), sizeof(exe));

    if (exe.t_size > _exe.size() - 0x800) {
        printf("Invalid exe t_size: 0x%08x\n", exe.t_size);
        return false;
    }

    for (size_t i = 0; i < exe.t_size; i++) {
        writeMemory8(exe.t_addr + i, _exe[0x800 + i]);
    }

    // PC = exe.pc0;
    // reg[28] = exe.gp0;
    // reg[29] = exe.s_addr + exe.s_size;
    // reg[30] = exe.s_addr + exe.s_size;

    exception = false;
    shouldJump = false;
    jumpPC = 0;

    return true;
}

bool CPU::loadBios(std::string path) {
    auto _bios = getFileContents(path);
    if (_bios.empty()) {
        printf("Cannot open BIOS %s", path.c_str());
        return false;
    }
    assert(_bios.size() == 512 * 1024);
    copy(_bios.begin(), _bios.end(), bios);
    state = State::run;
    return true;
}

bool CPU::loadExpansion(std::string path) {
    auto _exp = getFileContents(path);
    if (_exp.empty()) {
        return false;
    }
    assert(_exp.size() < EXPANSION_SIZE);
    copy(_exp.begin(), _exp.end(), expansion);
    return true;
}

void CPU::dumpRam() {
    std::vector<uint8_t> ram;
    ram.assign(this->ram, this->ram + 0x1fffff);
    putFileContents("ram.bin", ram);
}
}  // namespace mips
