#pragma once
#include "device.h"

class Serial {
    static const uint32_t BASE_ADDRESS = 0x1F801050;
    Reg32 status;
    Reg16 baud;

    void reset();

   public:
    Serial();
    void step();
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);
};