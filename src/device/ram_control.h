#pragma once
#include "device.h"

class RamControl {
    bool verbose = false;
    Reg32 ramSize;

   public:
    RamControl();
    void reset();
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);

    template <class Archive>
    void serialize(Archive& ar) {
        ar(ramSize);
    }
};