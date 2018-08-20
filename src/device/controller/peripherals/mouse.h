#pragma once
#include <string>
#include "abstract_device.h"
#include "device/device.h"

namespace peripherals {
struct Mouse : public AbstractDevice {
    union ButtonState {
        struct {
            uint16_t  : 8;
            uint16_t  : 2;
            uint16_t right : 1;
            uint16_t left : 1;
            uint16_t : 4;
        };

        uint16_t _reg;
        uint8_t _byte[2];

        ButtonState()  {
            _reg = 0xfcff;
            right = 1;
            left = 1;
        }
    };
    int verbose;

    ButtonState buttons;
    int8_t x = 0, y = 0;

    uint8_t handle(uint8_t byte) override;
    Mouse();
};
};  // namespace peripherals