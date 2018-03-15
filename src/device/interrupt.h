#pragma once
#include <string>
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
        Bit vblank : 1;      // IRQ0
        Bit gpu : 1;         // IRQ1
        Bit cdrom : 1;       // IRQ2
        Bit dma : 1;         // IRQ3
        Bit timer0 : 1;      // IRQ4
        Bit timer1 : 1;      // IRQ5
        Bit timer2 : 1;      // IRQ6
        Bit controller : 1;  // IRQ7
        Bit sio : 1;         // IRQ8
        Bit spu : 1;         // IRQ9
        Bit lightpen : 1;    // IRQ10
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
    std::string getMask();
    std::string getStatus();
};