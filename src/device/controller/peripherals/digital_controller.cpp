#include "digital_controller.h"

namespace peripherals {
void DigitalController::ButtonState::setByName(const std::string& name, bool value) {
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

        case 2: state++; return 0x5a;

        case 3: state++; return ~buttons._byte[0];

        case 4: state = 0; return ~buttons._byte[1];

        default: state = 0; return 0xff;
    }
}

bool DigitalController::getAck() { return state != 0; }

void DigitalController::resetState() {
    state = 0;
}

};  // namespace peripherals