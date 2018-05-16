#pragma once
#include "device.h"

class MemoryControl {
    Reg32 status;

    void reset();

   public:
    MemoryControl();
    uint32_t read(uint32_t address);
    void write(uint32_t address, uint32_t data);
};