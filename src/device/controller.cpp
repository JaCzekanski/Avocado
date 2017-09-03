#include "controller.h"
#include "mips.h"

namespace device {
namespace controller {
void DigitalController::setByName(std::string& name, bool value) {
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

uint8_t DigitalController::handle(uint8_t byte) {
    switch (state) {
        case 0:
            if (byte == 0x01) {
                state++;
                return 0xff;
            }
            return 0xff;

        case 1:
            if (byte == 0x42) {
                state++;
                return 0x41;
            }
            state = 0;
            return 0xff;

        case 2:
            state++;
            return 0x5a;

        case 3:
            state++;
            return ~_byte[0];

        case 4:
            state = 0;
            return ~_byte[1];

        default:
            state = 0;
            return 0xff;
    }
}

bool DigitalController::getAck() { return state != 0; }

void Controller::handleByte(uint8_t byte) {
    rxPending = true;

    if ((control._reg & (1 << 13)) == 0) {
        // Port 1
        rxData = state.handle(byte);
        ack = state.getAck();
        if (state.getAck()) {
            irqTimer = 3;
        }
    } else {
        // Port 2
        //		rxData = state.handle(byte);
        //		ack = state.getAck();
        //		if (state.getAck()) {
        //			irqTimer = 3;
        //		}
    }
}

Controller::Controller() {}

void Controller::step() {
    if (irqTimer > 0) {
        if (--irqTimer == 0) {
            irq = true;
        }
    }
    if (irq) {
        ((mips::CPU*)_cpu)->interrupt->trigger(interrupt::CONTROLLER);
    }
}

uint8_t Controller::read(uint32_t address) {
    if (address == 0) {  // RX
        return getData();
    }
    if (address == 4) {
        uint8_t data = (ack << 7) |        // /ACK Input Level 0 - High, 1 - Low
                                           // 6, 5, 4 - 0
                       (0 << 3) |          // Parity error
                       (0 << 2) |          // TX Ready Flag 2
                       (rxPending << 1) |  // RX FIFO Not Empty
                       (1 << 0);           // TX Ready Flag 1
        ack = 0;
        return data;
    }
    if (address == 5) {
        return (irq << 1);
    }
    if (address >= 8 && address < 10) return mode._byte[address - 8];
    if (address >= 10 && address < 12) return control._byte[address - 10];
    if (address >= 14 && address < 16) return baud._byte[address - 14];
    return 0xff;
}

void Controller::write(uint32_t address, uint8_t data) {
    if (address == 0) {
        handleByte(data);
    } else if (address >= 8 && address < 10) {
        mode._byte[address - 8] = data;
    } else if (address >= 10 && address < 12) {
        control._byte[address - 10] = data;  // mask bits 4 & 6

        if (address == 10 && (data & 0x10)) {
            irq = false;
        }
    } else if (address >= 14 && address < 16) {
        baud._byte[address - 14] = data;
    }
}
}
}
