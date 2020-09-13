#include "digital_controller.h"
#include <fmt/core.h>
#include "config.h"
#include "input/input_manager.h"
#include "freecam_controller.h"

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

DigitalController::DigitalController(Type type, int port)
    : AbstractDevice(type, port), verbose(config.debug.log.controller), path(fmt::format("controller/{}/", port)) {}

DigitalController::DigitalController(int port) : DigitalController(Type::Digital, port) {}

uint8_t DigitalController::_handle(uint8_t byte) {
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

        default: return handleRead(byte);
    }
}

uint8_t DigitalController::handle(uint8_t byte) {
    int _state = state;
    uint8_t response = _handle(byte);
    if (verbose >= 1) fmt::print("[DIGITAL_{}] data: 0x{:02x}, resp: 0x{:02x}, (state: {})\n", port, byte, response, _state);
    return response;
}

uint8_t DigitalController::handleRead(uint8_t byte) {
    (void)byte;
    switch (state) {
        case 2: state++; return 0x5a;
        case 3: state++; return ~buttons._byte[0];
        case 4: state = 0; return ~buttons._byte[1];

        default: return 0xff;
    }
}

void DigitalController::update() {
    auto inputManager = InputManager::getInstance();
    if (inputManager == nullptr) return;

    buttons.select = inputManager->getDigital(path + "select");
    buttons.start = inputManager->getDigital(path + "start");
    buttons.up = inputManager->getDigital(path + "dpad_up");
    buttons.right = inputManager->getDigital(path + "dpad_right");
    buttons.down = inputManager->getDigital(path + "dpad_down");
    buttons.left = inputManager->getDigital(path + "dpad_left");
    buttons.l2 = inputManager->getDigital(path + "l2");
    buttons.r2 = inputManager->getDigital(path + "r2");
    buttons.l1 = inputManager->getDigital(path + "l1");
    buttons.r1 = inputManager->getDigital(path + "r1");
    buttons.triangle = inputManager->getDigital(path + "triangle");
    buttons.circle = inputManager->getDigital(path + "circle");
    buttons.cross = inputManager->getDigital(path + "cross");
    buttons.square = inputManager->getDigital(path + "square");

    FreecamController::update(path);
}
};  // namespace peripherals