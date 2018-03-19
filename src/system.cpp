#include "system.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "bios/functions.h"
#include "platform/windows/config.h"
#include "utils/file.h"
#include "utils/psx_exe.h"

System::System() {
    memset(bios, 0, BIOS_SIZE);
    memset(ram, 0, RAM_SIZE);
    memset(scratchpad, 0, SCRATCHPAD_SIZE);
    memset(expansion, 0, EXPANSION_SIZE);

    cpu = std::make_unique<mips::CPU>(this);
    memoryControl = std::make_unique<device::Dummy>("MemCtrl", 0x1f801000, false);
    controller = std::make_unique<device::controller::Controller>(this);
    serial = std::make_unique<device::Dummy>("Serial", 0x1f801050, false);
    interrupt = std::make_unique<Interrupt>(this);
    gpu = std::make_unique<GPU>();
    dma = std::make_unique<device::dma::DMA>(this);
    timer0 = std::make_unique<Timer<0>>(this);
    timer1 = std::make_unique<Timer<1>>(this);
    timer2 = std::make_unique<Timer<2>>(this);
    cdrom = std::make_unique<device::cdrom::CDROM>(this);
    spu = std::make_unique<SPU>();
    mdec = std::make_unique<MDEC>();
    expansion2 = std::make_unique<device::Dummy>("Expansion2", 0x1f802000, false);

    biosLog = config["debug"]["log"]["bios"];
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

/*
Based on http://problemkaputt.de/psx-spx.htm
0x00000000 - 0x20000000 is cache enabled
0x80000000 is mirrored to 0x00000000 (with cache)
0xA0000000 is mirrored to 0x00000000 (without cache)

00000000 - 00200000 2048K  RAM
1F000000 - 1F800000 8192K  Expansion ROM
1F800000 - 1F800400 1K     Scratchpad
1F801000 - 1F803000 8K     I/O
1F802000 - 1F804000 8K     Expansion 2
1FA00000 - 1FC00000 2048K  Expansion 3
1FC00000 - 1FC80000 512K   BIOS ROM
FFFE0000 - FFFE0100 0.5K   I/O (Cache Control)
*/

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

#ifdef ENABLE_IO_LOG
#define LOG_IO(mode, size, addr, data) ioLogList.push_back({(mode), (size), (addr), (data)})
#else
#define LOG_IO(mode, size, addr, data)
#endif

#define READ_IO(begin, end, periph)                                     \
    if (addr >= (begin) && addr < (end)) {                              \
        auto data = read_io<T>((periph), addr - (begin));               \
                                                                        \
        LOG_IO(IO_LOG_ENTRY::MODE::READ, sizeof(T) * 8, address, data); \
        return data;                                                    \
    }

#define READ_IO32(begin, end, periph)                                                                          \
    if (addr >= (begin) && addr < (end)) {                                                                     \
        T data = 0;                                                                                            \
        if (sizeof(T) == 4)                                                                                    \
            data = (periph)->read(addr - (begin));                                                             \
        else                                                                                                   \
            printf("R Unsupported access to " #periph " with bit size %d\n", static_cast<int>(sizeof(T) * 8)); \
                                                                                                               \
        LOG_IO(IO_LOG_ENTRY::MODE::READ, sizeof(T) * 8, address, data);                                        \
        return data;                                                                                           \
    }

#define WRITE_IO(begin, end, periph)                                     \
    if (addr >= (begin) && addr < (end)) {                               \
        write_io<T>((periph), addr - (begin), data);                     \
                                                                         \
        LOG_IO(IO_LOG_ENTRY::MODE::WRITE, sizeof(T) * 8, address, data); \
        return;                                                          \
    }

#define WRITE_IO32(begin, end, periph)                                                                         \
    if (addr >= (begin) && addr < (end)) {                                                                     \
        if (sizeof(T) == 4)                                                                                    \
            (periph)->write(addr - (begin), data);                                                             \
        else                                                                                                   \
            printf("W Unsupported access to " #periph " with bit size %d\n", static_cast<int>(sizeof(T) * 8)); \
                                                                                                               \
        LOG_IO(IO_LOG_ENTRY::MODE::WRITE, sizeof(T) * 8, address, data);                                       \
        return;                                                                                                \
    }

template <typename T>
INLINE T System::readMemory(uint32_t address) {
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

    // IO Ports
    if (addr >= 0x1f801000 && addr <= 0x1f803000) {
        addr -= 0x1f801000;

        if (addr >= 0x50 && addr < 0x60) {
            if (addr >= 0x54 && addr < 0x58) {
                return 0x5 >> ((addr - 0x54) * 8);
            }
        }

        READ_IO(0x00, 0x24, memoryControl);
        READ_IO(0x40, 0x50, controller);
        READ_IO(0x50, 0x60, serial);
        READ_IO(0x60, 0x64, memoryControl);
        READ_IO(0x70, 0x78, interrupt);
        READ_IO(0x80, 0x100, dma);
        READ_IO(0x100, 0x110, timer0);
        READ_IO(0x110, 0x120, timer1);
        READ_IO(0x120, 0x130, timer2);
        READ_IO(0x800, 0x804, cdrom);
        READ_IO32(0x810, 0x0818, gpu);
        READ_IO32(0x820, 0x828, mdec);
        READ_IO(0xC00, 0x1000, spu);
        READ_IO(0x1000, 0x1043, expansion2);
        printf("R Unhandled IO at 0x%08x\n", addr + 0x1f801000);
    }

    if (address >= 0xfffe0130 && address < 0xfffe0134) {
        printf("R Unhandled memory control\n");
        return 0;
    }

    printf("R Unhandled address at 0x%08x\n", address);
    return 0;
}
template <typename T>
INLINE void System::writeMemory(uint32_t address, T data) {
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
        if (cpu->cop0.status.isolateCache) return;
        return write_fast<T>(ram, addr & 0x1fffff, data);
    }
    if (addr >= 0x1f000000 && addr < 0x1f000000 + EXPANSION_SIZE) {
        return write_fast<T>(expansion, addr - 0x1f000000, data);
    }
    if (addr >= 0x1f800000 && addr < 0x1f800400) {
        return write_fast<T>(scratchpad, addr - 0x1f800000, data);
    }

    // IO Ports
    if (addr >= 0x1f801000 && addr <= 0x1f803000) {
        addr -= 0x1f801000;

        WRITE_IO(0x00, 0x24, memoryControl);
        WRITE_IO(0x40, 0x50, controller);
        WRITE_IO(0x50, 0x60, serial);
        WRITE_IO(0x60, 0x64, memoryControl);
        WRITE_IO(0x70, 0x78, interrupt);
        WRITE_IO(0x80, 0x100, dma);
        WRITE_IO(0x100, 0x110, timer0);
        WRITE_IO(0x110, 0x120, timer1);
        WRITE_IO(0x120, 0x130, timer2);
        WRITE_IO(0x800, 0x804, cdrom);
        WRITE_IO32(0x810, 0x0818, gpu);
        WRITE_IO32(0x820, 0x828, mdec);
        WRITE_IO(0xC00, 0x1000, spu);
        WRITE_IO(0x1000, 0x1043, expansion2);
        printf("W Unhandled IO at 0x%08x: 0x%02x\n", addr, data);
    }

    // Cache control
    if (address >= 0xfffe0130 && address < 0xfffe0134) {
        printf("W Unhandled memory control\n");
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

void System::printFunctionInfo(int type, uint8_t number, bios::Function f) {
    printf("  BIOS %02X(%02x): %s(", type, number, f.name);
    for (int i = 0; i < f.argc; i++) {
        if (i > 4) break;
        printf("0x%x%s", cpu->reg[4 + i], i == (f.argc - 1) ? "" : ", ");
    }
    printf(")\n");
}

void System::handleBiosFunction() {
    std::unordered_map<uint8_t, bios::Function>::const_iterator function;
    uint32_t maskedPC = cpu->PC & 0x1FFFFF;
    uint8_t functionNumber = cpu->reg[9];
    bool log = biosLog;

    if (maskedPC == 0xA0) {
        function = bios::A0.find(functionNumber);
        if (function == bios::A0.end()) return;
        if (function->second.callback != nullptr) log = function->second.callback(this);
    } else if (maskedPC == 0xB0) {
        function = bios::B0.find(functionNumber);
        if (function == bios::B0.end()) return;
        if (function->second.callback != nullptr) log = function->second.callback(this);
    } else {
        if (log)
            printf("  BIOS %02X(%02x) (0x%02x, 0x%02x, 0x%02x, 0x%02x)\n", maskedPC >> 4, functionNumber, cpu->reg[4], cpu->reg[5],
                   cpu->reg[6], cpu->reg[7]);
        return;
    }

    if (log) printFunctionInfo(maskedPC >> 4, function->first, function->second);
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

        static int wtf = 0;
        if (++wtf % 200 == 0) interrupt->trigger(interrupt::TIMER2);
        controller->step();

        if (gpu->emulateGpuCycles(systemCycles)) {
            interrupt->trigger(interrupt::VBLANK);
            return;  // frame emulated
        }
    }
}

void System::softReset() {
    printf("Soft reset\n");
    cpu->PC = 0xBFC00000;
    cpu->shouldJump = false;
    state = State::run;
}

bool System::loadExeFile(std::string exePath) {
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

    cpu->PC = exe.pc0;
    cpu->reg[28] = exe.gp0;
    cpu->reg[29] = exe.s_addr + exe.s_size;
    cpu->reg[30] = exe.s_addr + exe.s_size;

    cpu->exception = false;
    cpu->shouldJump = false;
    cpu->jumpPC = 0;

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

    copy(_bios.begin(), _bios.end(), bios);
    state = State::run;
    return true;
}

bool System::loadExpansion(std::string path) {
    const char* licenseString = "Licensed by Sony Computer Entertainment Inc";

    auto _exp = getFileContents(path);
    if (_exp.empty()) {
        return false;
    }

    assert(_exp.size() <= EXPANSION_SIZE);

    if (memcmp(_exp.data() + 4, licenseString, strlen(licenseString)) != 0) {
        printf("[WARNING]: Loaded expansion (%s) have invalid header, are you using correct file?\n", getFilenameExt(path).c_str());
    }

    copy(_exp.begin(), _exp.end(), expansion);
    return true;
}

void System::dumpRam() {
    std::vector<uint8_t> ram;
    ram.assign(this->ram, this->ram + 0x1fffff);
    putFileContents("ram.bin", ram);
}