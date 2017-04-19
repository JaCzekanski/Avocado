#pragma once
#include "device.h"
#include <deque>

namespace device {
namespace spu {

struct Voice {
    Reg32 volume;
    Reg16 sampleRate;
    Reg16 startAddress;
    Reg32 ADSR;
    Reg16 ADSRVolume;
    Reg16 repeatAddress;
};

class SPU : public Device {
    static const uint32_t BASE_ADDRESS = 0x1f801c00;
    static const int VOICE_COUNT = 24;
    void *_cpu = nullptr;

    Voice voices[VOICE_COUNT];
    Reg16 SPUCNT;
    Reg16 SPUSTAT;

   public:
    SPU();
    void step();
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);

    void setCPU(void *cpu) { this->_cpu = cpu; }
};
}
}