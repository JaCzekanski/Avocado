#pragma once
#include "device.h"

class MemoryControl {
    bool verbose = false;
    Reg32 exp1Base;
    Reg32 exp2Base;
    Reg32 exp1Config;
    Reg32 exp2Config;
    Reg32 exp3Config;
    Reg32 biosConfig;
    Reg32 spuConfig;
    Reg32 cdromConfig;

   public:
    MemoryControl();
    void reset();
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);

    template <class Archive>
    void serialize(Archive& ar) {
        ar(exp1Base);
        ar(exp2Base);
        ar(exp1Config);
        ar(exp2Config);
        ar(exp3Config);
        ar(biosConfig);
        ar(spuConfig);
        ar(cdromConfig);
    }
};