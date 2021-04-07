#include "memory_control.h"
#include <fmt/core.h>
#include <config.h>

MemoryControl::MemoryControl() {
    verbose = config.debug.log.memoryControl > 0;
    reset();
}

void MemoryControl::reset() {
    // TODO: Check reset values
    exp1Base._reg = 0;
    exp2Base._reg = 0;
    exp1Config._reg = 0;
    exp3Config._reg = 0;
    biosConfig._reg = 0;
    spuConfig._reg = 0;
    cdromConfig._reg = 0;
    exp2Config._reg = 0;
}

#define ADDR (address + 0x1f801000)
#define READ(a, reg)                                                                                  \
    if (ADDR >= (a) && ADDR < (a) + 4) {                                                              \
        if (verbose && (ADDR & 3) == 3) fmt::print("[MEMCTRL] R @ " #reg ": 0x{:08x}\n", (reg)._reg); \
        return (reg).read(ADDR - (a));                                                                \
    }
#define WRITE(a, reg)                                                                                 \
    if (ADDR >= (a) && ADDR < (a) + 4) {                                                              \
        (reg).write(ADDR - (a), data);                                                                \
        if (verbose && (ADDR & 3) == 3) fmt::print("[MEMCTRL] W @ " #reg ": 0x{:08x}\n", (reg)._reg); \
        return;                                                                                       \
    }

uint8_t MemoryControl::read(uint32_t address) {
    READ(0x1F801000, exp1Base);
    READ(0x1F801004, exp2Base);
    READ(0x1F801008, exp1Config);
    READ(0x1F80100C, exp3Config);
    READ(0x1F801010, biosConfig);
    READ(0x1F801014, spuConfig);
    READ(0x1F801018, cdromConfig);
    READ(0x1F80101C, exp2Config);
    return 0;
}

void MemoryControl::write(uint32_t address, uint8_t data) {
    WRITE(0x1F801000, exp1Base);
    WRITE(0x1F801004, exp2Base);
    WRITE(0x1F801008, exp1Config);
    WRITE(0x1F80100C, exp3Config);
    WRITE(0x1F801010, biosConfig);
    WRITE(0x1F801014, spuConfig);
    WRITE(0x1F801018, cdromConfig);
    WRITE(0x1F80101C, exp2Config);
}