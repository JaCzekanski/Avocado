#pragma once
#include "device.h"
#include <deque>

class MDEC {
    static const uint32_t BASE_ADDRESS = 0x1f801820;
    Reg32 command;
    Reg32 data;
    Reg32 status;
    Reg32 _control;

    bool color = false;  // 0 - luminance only, 1 - luminance and color
    uint8_t cmd = 0;
    int paramCount = 0;

    void reset();

   public:
    MDEC();
    void step();
    uint8_t read(uint32_t address);
    void handleCommand(uint8_t cmd, uint32_t data);
    void write(uint32_t address, uint8_t data);
};