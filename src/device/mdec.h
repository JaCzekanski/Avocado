#pragma once
#include "device.h"
#include <deque>

namespace device {
namespace mdec {

class MDEC : public Device {
    static const uint32_t BASE_ADDRESS = 0x1f801820;
    Reg32 command;
    Reg32 data;
    Reg32 status;
    Reg32 _control;

    void *_cpu = nullptr;

    void reset();
   public:
    MDEC();
    void step();
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);

    void setCPU(void *cpu) { this->_cpu = cpu; }
};
}
}
