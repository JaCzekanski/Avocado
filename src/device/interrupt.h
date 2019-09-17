#pragma once
#include "device.h"

struct System;

namespace interrupt {
enum IrqNumber {
    VBLANK = 0,
    GPU = 1,
    CDROM = 2,
    DMA = 3,
    TIMER0 = 4,
    TIMER1 = 5,
    TIMER2 = 6,
    CONTROLLER = 7,
    SIO = 8,
    SPU = 9,
    LIGHTPEN = 10
};

// Interrupt Channels
union IRQ {
    struct {
        uint16_t vblank : 1;      // IRQ0
        uint16_t gpu : 1;         // IRQ1
        uint16_t cdrom : 1;       // IRQ2
        uint16_t dma : 1;         // IRQ3
        uint16_t timer0 : 1;      // IRQ4
        uint16_t timer1 : 1;      // IRQ5
        uint16_t timer2 : 1;      // IRQ6
        uint16_t controller : 1;  // IRQ7
        uint16_t sio : 1;         // IRQ8
        uint16_t spu : 1;         // IRQ9
        uint16_t lightpen : 1;    // IRQ10
        uint16_t : 5;
    };
    uint16_t _reg;
    uint8_t _byte[2];

    IRQ() : _reg(0) {}
};
}  // namespace interrupt

class Interrupt {
    interrupt::IRQ status;
    interrupt::IRQ mask;

    System* sys;

   public:
    Interrupt(System* sys);
    void step();
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);

    void trigger(interrupt::IrqNumber irq);
    bool interruptPending();

    template <class Archive>
    void serialize(Archive& ar) {
        ar(status._reg, mask._reg);
    }
};