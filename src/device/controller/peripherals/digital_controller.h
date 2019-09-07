#pragma once
#include <string>
#include "abstract_device.h"
#include "device/device.h"

namespace peripherals {
class DigitalController : public AbstractDevice {
   protected:
    union ButtonState {
        struct {
            uint16_t select : 1;
            uint16_t l3 : 1;  // L3 and R3 are not used by digital controller
            uint16_t r3 : 1;
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
        ButtonState() : _reg(0) {}
    };

    int verbose;
    ButtonState buttons;
    std::string path;

    DigitalController(Type type, int port);
    uint8_t _handle(uint8_t byte);
    uint8_t handleRead(uint8_t byte);

   public:
    DigitalController(int Port);
    uint8_t handle(uint8_t byte) override;
    void update() override;
};
};  // namespace peripherals