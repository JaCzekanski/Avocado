#pragma once
#include "device.h"
#include <deque>

namespace device {
namespace controller {

class DigitalController {
    int state = 0;

   public:
    union {
        struct {
            Bit select : 1;
            Bit : 1;
            Bit : 1;
            Bit start : 1;
            Bit up : 1;
            Bit right : 1;
            Bit down : 1;
            Bit left : 1;
            Bit l2 : 1;
            Bit r2 : 1;
            Bit l1 : 1;
            Bit r1 : 1;
            Bit triangle : 1;
            Bit circle : 1;
            Bit cross : 1;
            Bit square : 1;
        };

        uint16_t _reg;
        uint8_t _byte[2];
    };

    void setByName(std::string &name, bool value) {
#define BUTTON(x)     \
    if (name == #x) { \
        x = value;    \
        return;       \
    }
        BUTTON(select)
        BUTTON(start)
        BUTTON(up)
        BUTTON(right)
        BUTTON(down)
        BUTTON(left)
        BUTTON(l2)
        BUTTON(r2)
        BUTTON(l1)
        BUTTON(r1)
        BUTTON(triangle)
        BUTTON(circle)
        BUTTON(cross)
        BUTTON(square)
#undef BUTTON
    }

    uint8_t handle(uint8_t byte);
    bool getAck();
    DigitalController() : _reg(0) {}
};

class Controller : public Device {
    const int baseAddress = 0x1f801040;
    void *_cpu = nullptr;
    DigitalController state;

    Reg16 mode;
    Reg16 control;
    Reg16 baud;
    bool irq = false;
    int irqTimer = 0;

    void handleByte(uint8_t byte);

    uint8_t rxData = 0;
    bool rxPending = false;
    bool ack = false;

    uint8_t getData() {
        uint8_t ret = rxData;
        rxData = 0xff;
        rxPending = false;

        return ret;
    }

   public:
    Controller();
    void step() override;
    uint8_t read(uint32_t address) override;
    void write(uint32_t address, uint8_t data) override;
    void setCPU(void *cpu) { this->_cpu = cpu; }
    void setState(DigitalController state) { this->state = state; }
};
}
}
