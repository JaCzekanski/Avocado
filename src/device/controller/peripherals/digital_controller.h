#pragma once
#include <string>
#include "device/device.h"

namespace peripherals {
struct DigitalController {
    union ButtonState {
        struct {
            uint16_t select : 1;
            uint16_t : 1;
            uint16_t : 1;
            uint16_t start : 1;
            uint16_t up : 1;
            uint16_t right : 1;
            uint16_t down : 1;
            uint16_t left : 1;
            uint16_t l2 : 1;
            uint16_t r2 : 1;
            uint16_t l1 : 1;
            uint16_t r1 : 1;
            uint16_t triangle : 1;
            uint16_t circle : 1;
            uint16_t cross : 1;
            uint16_t square : 1;
        };

        uint16_t _reg;
        uint8_t _byte[2];

        void setByName(const std::string& name, bool value);
        ButtonState(uint16_t reg) : _reg(reg) {}
        ButtonState() = default;
    };

    int state = 0;
    ButtonState buttons;

    uint8_t handle(uint8_t byte);
    bool getAck();
    void resetState();
    DigitalController() : buttons(0) {}
};
};  // namespace peripherals