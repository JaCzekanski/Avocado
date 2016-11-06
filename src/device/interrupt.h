#pragma once
#include "device.h"
#include <string>

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
        step();
    }

    bool interruptPending() { return (status._reg & mask._reg) ? true : false; }
    std::string getMask() {
        std::string buf;
        buf.resize(11);
        buf[0] = mask.vblank ? 'V' : '-';
        buf[1] = mask.gpu ? 'G' : '-';
        buf[2] = mask.cdrom ? 'C' : '-';
        buf[3] = mask.dma ? 'D' : '-';
        buf[4] = mask.timer0 ? '0' : '-';
        buf[5] = mask.timer1 ? '1' : '-';
        buf[6] = mask.timer2 ? '2' : '-';
        buf[7] = mask.controller ? 'C' : '-';
        buf[8] = mask.sio ? 'I' : '-';
        buf[9] = mask.spu ? 'S' : '-';
        buf[10] = mask.lightpen ? 'L' : '-';
        buf[11] = 0;
        return buf;
    }
    std::string getStatus() {
        std::string buf;
        buf.resize(11);
        buf[0] = status.vblank ? 'V' : '-';
        buf[1] = status.gpu ? 'G' : '-';
        buf[2] = status.cdrom ? 'C' : '-';
        buf[3] = status.dma ? 'D' : '-';
        buf[4] = status.timer0 ? '0' : '-';
        buf[5] = status.timer1 ? '1' : '-';
        buf[6] = status.timer2 ? '2' : '-';
        buf[7] = status.controller ? 'C' : '-';
        buf[8] = status.sio ? 'I' : '-';
        buf[9] = status.spu ? 'S' : '-';
        buf[10] = status.lightpen ? 'L' : '-';
        buf[11] = 0;
        return buf;
    }

    void setCPU(void *cpu) { this->_cpu = cpu; }
};
}
}