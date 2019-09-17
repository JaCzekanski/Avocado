#pragma once
#include <cstdint>

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

    template <class Archive>
    void serialize(Archive& ar) {
        ar(_reg);
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

    void setBit(int n, bool v) {
        if (n >= 32) return;
        _reg &= ~(1 << n);
        _reg |= (v << n);
    }

    bool getBit(int n) {
        if (n >= 32) return false;
        return (_reg & (1 << n)) != 0;
    }

    template <class Archive>
    void serialize(Archive& ar) {
        ar(_reg);
    }
};

namespace mips {
struct CPU;
}