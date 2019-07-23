#include "system.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "bios/functions.h"
#include "config.h"
#include "sound/sound.h"
#include "utils/address.h"
#include "utils/file.h"
#include "utils/psx_exe.h"

System::System() {
    memset(bios, 0, BIOS_SIZE);
    memset(ram, 0, RAM_SIZE);
    memset(scratchpad, 0, SCRATCHPAD_SIZE);
    memset(expansion, 0, EXPANSION_SIZE);

    cpu = std::make_unique<mips::CPU>(this);
    gpu = std::make_unique<gpu::GPU>();
    spu = std::make_unique<spu::SPU>(this);
    mdec = std::make_unique<mdec::MDEC>();

    cdrom = std::make_unique<device::cdrom::CDROM>(this);
    controller = std::make_unique<device::controller::Controller>(this);
    dma = std::make_unique<device::dma::DMA>(this);
    expansion2 = std::make_unique<Expansion2>();
    interrupt = std::make_unique<Interrupt>(this);
    memoryControl = std::make_unique<MemoryControl>();
    serial = std::make_unique<Serial>();
    timer0 = std::make_unique<Timer<0>>(this);
    timer1 = std::make_unique<Timer<1>>(this);
    timer2 = std::make_unique<Timer<2>>(this);

    debugOutput = config["debug"]["log"]["system"].get<int>();
    biosLog = config["debug"]["log"]["bios"];
}

// Note: stupid static_casts and asserts are only to supress MSVC warnings

// Warning: This function does not check array boundaries. Make sure that address is aligned!
template <typename T>
constexpr T read_fast(uint8_t* device, uint32_t addr) {
    static_assert(std::is_same<T, uint8_t>() || std::is_same<T, uint16_t>() || std::is_same<T, uint32_t>(), "Invalid type used");

    if (sizeof(T) == 1) return static_cast<T>(((uint8_t*)device)[addr]);
    if (sizeof(T) == 2) return static_cast<T>(((uint16_t*)device)[addr / 2]);
    if (sizeof(T) == 4) return static_cast<T>(((uint32_t*)device)[addr / 4]);
    return 0;
}

// Warning: This function does not check array boundaries. Make sure that address is aligned!
template <typename T>
constexpr void write_fast(uint8_t* device, uint32_t addr, T value) {
    static_assert(std::is_same<T, uint8_t>() || std::is_same<T, uint16_t>() || std::is_same<T, uint32_t>(), "Invalid type used");

    if (sizeof(T) == 1)
        ((uint8_t*)device)[addr] = static_cast<uint8_t>(value);
    else if (sizeof(T) == 2)
        ((uint16_t*)device)[addr / 2] = static_cast<uint16_t>(value);
    else if (sizeof(T) == 4)
        ((uint32_t*)device)[addr / 4] = static_cast<uint32_t>(value);
}

template <typename T, typename Device>
constexpr T read_io(Device& periph, uint32_t addr) {
    static_assert(std::is_same<T, uint8_t>() || std::is_same<T, uint16_t>() || std::is_same<T, uint32_t>(), "Invalid type used");

    if (sizeof(T) == 1) return periph->read(addr);
    if (sizeof(T) == 2) return periph->read(addr) | periph->read(addr + 1) << 8;
    if (sizeof(T) == 4)
        return periph->read(addr) | periph->read(addr + 1) << 8 | periph->read(addr + 2) << 16 | periph->read(addr + 3) << 24;
    return 0;
}

template <typename T, typename Device>
constexpr void write_io(Device& periph, uint32_t addr, T data) {
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

#ifdef ENABLE_IO_LOG
#define LOG_IO(mode, size, addr, data, pc) ioLogList.push_back({(mode), (size), (addr), (data), (pc)})
#else
#define LOG_IO(mode, size, addr, data, pc)
#endif

#define READ_IO(begin, end, periph)                                              \
    if (addr >= (begin) && addr < (end)) {                                       \
        auto data = read_io<T>((periph), addr - (begin));                        \
                                                                                 \
        LOG_IO(IO_LOG_ENTRY::MODE::READ, sizeof(T) * 8, address, data, cpu->PC); \
        return data;                                                             \
    }

#define READ_IO32(begin, end, periph)                                                                          \
    if (addr >= (begin) && addr < (end)) {                                                                     \
        T data = 0;                                                                                            \
        if (sizeof(T) == 4) {                                                                                  \
            data = (periph)->read(addr - (begin));                                                             \
        } else {                                                                                               \
            printf("R Unsupported access to " #periph " with bit size %d\n", static_cast<int>(sizeof(T) * 8)); \
        }                                                                                                      \
                                                                                                               \
        LOG_IO(IO_LOG_ENTRY::MODE::READ, sizeof(T) * 8, address, data, cpu->PC);                               \
        return data;                                                                                           \
    }

#define WRITE_IO(begin, end, periph)                                              \
    if (addr >= (begin) && addr < (end)) {                                        \
        write_io<T>((periph), addr - (begin), data);                              \
                                                                                  \
        LOG_IO(IO_LOG_ENTRY::MODE::WRITE, sizeof(T) * 8, address, data, cpu->PC); \
        return;                                                                   \
    }

#define WRITE_IO32(begin, end, periph)                                                                         \
    if (addr >= (begin) && addr < (end)) {                                                                     \
        if (sizeof(T) == 4) {                                                                                  \
            (periph)->write(addr - (begin), data);                                                             \
        } else {                                                                                               \
            printf("W Unsupported access to " #periph " with bit size %d\n", static_cast<int>(sizeof(T) * 8)); \
        }                                                                                                      \
                                                                                                               \
        LOG_IO(IO_LOG_ENTRY::MODE::WRITE, sizeof(T) * 8, address, data, cpu->PC);                              \
        return;                                                                                                \
    }

template <typename T>
INLINE T System::readMemory(uint32_t address) {
    static_assert(std::is_same<T, uint8_t>() || std::is_same<T, uint16_t>() || std::is_same<T, uint32_t>(), "Invalid type used");

    uint32_t addr = align_mips<T>(address);

    if (in_range<RAM_BASE, RAM_SIZE * 4>(addr)) {
        return read_fast<T>(ram, (addr - RAM_BASE) & (RAM_SIZE - 1));
    }
    if (in_range<EXPANSION_BASE, EXPANSION_SIZE>(addr)) {
        return read_fast<T>(expansion, addr - EXPANSION_BASE);
    }
    if (in_range<SCRATCHPAD_BASE, SCRATCHPAD_SIZE>(addr)) {
        return read_fast<T>(scratchpad, addr - SCRATCHPAD_BASE);
    }
    if (in_range<BIOS_BASE, BIOS_SIZE>(addr)) {
        return read_fast<T>(bios, addr - BIOS_BASE);
    }

    READ_IO(0x1f801000, 0x1f801024, memoryControl);
    READ_IO(0x1f801040, 0x1f801050, controller);
    READ_IO(0x1f801050, 0x1f801060, serial);
    READ_IO(0x1f801060, 0x1f801064, memoryControl);
    READ_IO(0x1f801070, 0x1f801078, interrupt);
    READ_IO(0x1f801080, 0x1f801100, dma);
    READ_IO(0x1f801100, 0x1f801110, timer0);
    READ_IO(0x1f801110, 0x1f801120, timer1);
    READ_IO(0x1f801120, 0x1f801130, timer2);
    READ_IO(0x1f801800, 0x1f801804, cdrom);
    READ_IO32(0x1f801810, 0x1f801818, gpu);
    READ_IO32(0x1f801820, 0x1f801828, mdec);
    READ_IO(0x1f801C00, 0x1f802000, spu);
    READ_IO(0x1f802000, 0x1f802067, expansion2);

    if (in_range<0xfffe0130, 4>(address)) {
        auto data = read_io<T>(memoryControl, address);
        LOG_IO(IO_LOG_ENTRY::MODE::READ, sizeof(T) * 8, address, data, cpu->PC);
        return data;
    }

    printf("R Unhandled address at 0x%08x\n", address);
    return 0;
}
template <typename T>
INLINE void System::writeMemory(uint32_t address, T data) {
    static_assert(std::is_same<T, uint8_t>() || std::is_same<T, uint16_t>() || std::is_same<T, uint32_t>(), "Invalid type used");

    if (unlikely(cpu->cop0.status.isolateCache)) return;

    uint32_t addr = align_mips<T>(address);

    if (in_range<RAM_BASE, RAM_SIZE * 4>(addr)) {
        return write_fast<T>(ram, (addr - RAM_BASE) & (RAM_SIZE - 1), data);
    }
    if (in_range<EXPANSION_BASE, EXPANSION_SIZE>(addr)) {
        return write_fast<T>(expansion, addr - EXPANSION_BASE, data);
    }
    if (in_range<SCRATCHPAD_BASE, SCRATCHPAD_SIZE>(addr)) {
        return write_fast<T>(scratchpad, addr - SCRATCHPAD_BASE, data);
    }

    WRITE_IO(0x1f801000, 0x1f801024, memoryControl);
    WRITE_IO(0x1f801040, 0x1f801050, controller);
    WRITE_IO(0x1f801050, 0x1f801060, serial);
    WRITE_IO(0x1f801060, 0x1f801064, memoryControl);
    WRITE_IO(0x1f801070, 0x1f801078, interrupt);
    WRITE_IO(0x1f801080, 0x1f801100, dma);
    WRITE_IO(0x1f801100, 0x1f801110, timer0);
    WRITE_IO(0x1f801110, 0x1f801120, timer1);
    WRITE_IO(0x1f801120, 0x1f801130, timer2);
    WRITE_IO(0x1f801800, 0x1f801804, cdrom);
    WRITE_IO32(0x1f801810, 0x1f801818, gpu);
    WRITE_IO32(0x1f801820, 0x1f801828, mdec);
    WRITE_IO(0x1f801C00, 0x1f802000, spu);
    WRITE_IO(0x1f802000, 0x1f802067, expansion2);
    WRITE_IO32(0xfffe0130, 0xfffe0134, memoryControl);

    if (in_range<0xfffe0130, 4>(address)) {
        write_io<T>(memoryControl, 0xfffe0130, data);
        LOG_IO(IO_LOG_ENTRY::MODE::WRITE, sizeof(T) * 8, address, data, cpu->PC);
        return;
    }

    printf("W Unhandled address at 0x%08x: 0x%02x\n", address, data);
}

uint8_t System::readMemory8(uint32_t address) { return readMemory<uint8_t>(address); }

uint16_t System::readMemory16(uint32_t address) { return readMemory<uint16_t>(address); }

uint32_t System::readMemory32(uint32_t address) { return readMemory<uint32_t>(address); }

void System::writeMemory8(uint32_t address, uint8_t data) { writeMemory<uint8_t>(address, data); }

void System::writeMemory16(uint32_t address, uint16_t data) { writeMemory<uint16_t>(address, data); }

void System::writeMemory32(uint32_t address, uint32_t data) { writeMemory<uint32_t>(address, data); }

void System::printFunctionInfo(const char* functionNum, const bios::Function& f) {
    printf("  %s: %s(", functionNum, f.name.c_str());
    int a = 0;
    for (auto arg : f.args) {
        uint32_t param = cpu->reg[4 + a];
        if (true) {
            printf("%s = ", arg.name.c_str());
        }
        switch (arg.type) {
            case bios::Type::INT:
            case bios::Type::POINTER: printf("0x%x", param); break;
            case bios::Type::CHAR: printf("'%c'", param); break;
            case bios::Type::STRING: {
                printf("\"");
                for (int i = 0; i < 32; i++) {
                    uint8_t c = readMemory8(param + i);

                    if (c == 0) {
                        break;
                    } else if (c != 0 && i == 32 - 1) {
                        printf("...");
                    } else {
                        printf("%c", isprint(c) ? c : '_');
                    }
                }
                printf("\"");
                break;
            }
        }
        if (a < (f.args.size() - 1)) {
            printf(", ");
        }
        a++;
        if (a > 4) break;
    }
    printf(")\n");
}

void System::handleBiosFunction() {
    uint32_t maskedPC = cpu->PC & 0x1FFFFF;
    uint8_t functionNumber = cpu->reg[9];
    bool log = biosLog;

    int tableNum = (maskedPC - 0xA0) / 0x10;
    if (tableNum > 2) return;

    const auto& table = bios::tables[tableNum];
    const auto& function = table.find(functionNumber);

    if (function == table.end()) return;
    if (function->second.callback != nullptr) {
        log = function->second.callback(this);
    }

    if (log) {
        std::string type = string_format("BIOS %1X(%02X)", 0xA + tableNum, functionNumber);
        printFunctionInfo(type.c_str(), function->second);
    }
}

void System::handleSyscallFunction() {
    uint8_t functionNumber = cpu->reg[4];
    bool log = biosLog;

    const auto& function = bios::SYSCALL.find(functionNumber);
    if (function == bios::SYSCALL.end()) return;

    if (function->second.callback != nullptr) {
        log = function->second.callback(this);
    }

    if (log) {
        std::string type = string_format("SYSCALL(%01X)", functionNumber);
        cpu->sys->printFunctionInfo(type.c_str(), function->second);
    }
}

void System::singleStep() {
    state = State::run;
    cpu->executeInstructions(1);
    state = State::pause;

    dma->step();
    cdrom->step();
    timer0->step(3);
    timer1->step(3);
    timer2->step(3);
    controller->step();
    spu->step(cdrom.get());

    if (gpu->emulateGpuCycles(3)) {
        interrupt->trigger(interrupt::VBLANK);
    }
}

void System::emulateFrame() {
#ifdef ENABLE_IO_LOG
    ioLogList.clear();
#endif
    cpu->gte.log.clear();
    gpu->gpuLogList.clear();

    gpu->prevVram = gpu->vram;
    int systemCycles = 300;
    for (;;) {
        if (!cpu->executeInstructions(systemCycles / 3)) {
            // printf("CPU Halted\n");
            return;
        }

        dma->step();
        cdrom->step();
        timer0->step(systemCycles);
        timer1->step(systemCycles);
        timer2->step(systemCycles);

        static float spuCounter = 0;
        // TODO: Yey, magic numbers!
        spuCounter += (float)systemCycles / 1.575f / (float)0x300;
        if (spuCounter >= 1.f) {
            spu->step(cdrom.get());
            spuCounter -= 1.0f;
        }

        if (spu->bufferReady) {
            spu->bufferReady = false;
            Sound::appendBuffer(spu->audioBuffer.begin(), spu->audioBuffer.end());
        }

        controller->step();

        if (gpu->emulateGpuCycles(systemCycles)) {
            interrupt->trigger(interrupt::VBLANK);
            return;  // frame emulated
        }

        if (gpu->gpuLine >= gpu::LINE_VBLANK_START_NTSC) {
            if (timer1->mode.syncEnabled) {
                auto mode1 = static_cast<timer::CounterMode::SyncMode1>(timer1->mode.syncMode);
                using modes = timer::CounterMode::SyncMode1;
                if (mode1 == modes::resetAtVblank || mode1 == modes::resetAtVblankAndPauseOutside) {
                    timer1->current._reg = 0;
                } else if (mode1 == modes::pauseUntilVblankAndFreerun) {
                    timer1->paused = false;
                    timer1->mode.syncEnabled = false;
                }
            }
        }
        // Handle Timer1 - Reset on VBlank
    }
}

void System::softReset() {
    printf("Soft reset\n");
    cpu->setPC(0xBFC00000);
    cpu->inBranchDelay = false;
    state = State::run;
}

bool System::loadExeFile(const std::vector<uint8_t>& _exe) {
    if (_exe.empty()) return false;
    assert(_exe.size() >= 0x800);

    PsxExe exe;
    memcpy(&exe, _exe.data(), sizeof(exe));

    if (exe.t_size > _exe.size() - 0x800) {
        printf("Invalid exe t_size: 0x%08x\n", exe.t_size);
        return false;
    }

    for (uint32_t i = 0; i < exe.t_size; i++) {
        writeMemory8(exe.t_addr + i, _exe[0x800 + i]);
    }

    cpu->setPC(exe.pc0);
    cpu->reg[28] = exe.gp0;
    cpu->reg[29] = exe.s_addr + exe.s_size;
    cpu->reg[30] = exe.s_addr + exe.s_size;

    cpu->exception = false;
    cpu->inBranchDelay = false;

    return true;
}

bool System::loadBios(std::string path) {
    const char* licenseString = "Sony Computer Entertainment Inc";

    auto _bios = getFileContents(path);
    if (_bios.empty()) {
        printf("Cannot open BIOS %s", path.c_str());
        return false;
    }
    assert(_bios.size() <= 512 * 1024);

    if (memcmp(_bios.data() + 0x108, licenseString, strlen(licenseString)) != 0) {
        printf("[WARNING]: Loaded bios (%s) have invalid header, are you using correct file?\n", getFilenameExt(path).c_str());
    }

    std::copy(_bios.begin(), _bios.end(), bios);
    state = State::run;
    return true;
}

bool System::loadExpansion(const std::vector<uint8_t>& _exp) {
    const char* licenseString = "Licensed by Sony Computer Entertainment Inc";

    if (_exp.empty()) {
        return false;
    }

    assert(_exp.size() <= EXPANSION_SIZE);

    if (memcmp(_exp.data() + 4, licenseString, strlen(licenseString)) != 0) {
        printf("[WARNING]: Loaded expansion have invalid header, are you using correct file?\n");
    }

    std::copy(_exp.begin(), _exp.end(), expansion);
    return true;
}

void System::dumpRam() {
    std::vector<uint8_t> ram;
    ram.assign(this->ram, this->ram + 0x1fffff);
    putFileContents("ram.bin", ram);
}
