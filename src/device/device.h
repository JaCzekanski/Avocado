#pragma once
#include <stdint.h>

namespace device {
typedef uint32_t Bit;

union Reg16 {
    uint16_t _reg;
    uint8_t _byte[2];

    Reg16() : _reg(0) {}
    void write(int n, uint8_t v) {
        if (n >= 2) return;
        _byte[n] = v;
    }
    uint8_t read(int n) const {
        if (n >= 2) return 0;
        return _byte[n];
    }
};

union Reg32 {
    uint32_t _reg;
    uint8_t _byte[4];

    Reg32() : _reg(0) {}
    void write(int n, uint8_t v) {
        if (n >= 4) return;
        _byte[n] = v;
    }
    uint8_t read(int n) const {
        if (n >= 4) return 0;
        return _byte[n];
    }
};

class Device {
   public:
    virtual ~Device(){};
    virtual void step() = 0;
    virtual uint8_t read(uint32_t address) = 0;
    virtual void write(uint32_t address, uint8_t data) = 0;
};
}
