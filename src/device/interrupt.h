#pragma once
#include "device.h"

namespace device {
namespace interrupt {

// Interrupt Channels
union IRQS {
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

    IRQS() : _reg(0) {}
};

class Interrupt : public Device {
    IRQS status;
    IRQS mask;

    void *_cpu = nullptr;

   public:
    Interrupt();
    void step();
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);

    void IRQ(int irq) {
        if (irq > 10) return;
        status._reg |= (1 << irq);
    }

    bool interruptPending() { return (status._reg & mask._reg) ? true : false; }

    void setCPU(void *cpu) { this->_cpu = cpu; }
};
}
}