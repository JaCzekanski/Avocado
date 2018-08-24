#include "mouse.h"
#include "config.h"
#include "input/input_manager.h"
#include "utils/math.h"
#include "utils/string.h"

namespace peripherals {
Mouse::Mouse(int port) : AbstractDevice(Type::Mouse, port), path(string_format("controller/%d/", port)) {}

uint8_t Mouse::handle(uint8_t byte) {
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
                return 0x12;
            }
            state = 0;
            return 0xff;

        case 2: state++; return 0x5a;
        case 3: state++; return 0xff;
        case 4: state++; return 0xf0 | ((!left) << 3) | ((!right) << 2);
        case 5: state++; return x;
        case 6: state = 0; return y;
        default: state = 0; return 0xff;
    }
}

void Mouse::update() {
    auto inputManager = InputManager::getInstance();
    if (inputManager == nullptr) return;

    left = inputManager->getDigital(path + "l1");
    right = inputManager->getDigital(path + "r1");

    auto inputUp = inputManager->getAnalog(path + "l_up");
    auto inputRight = inputManager->getAnalog(path + "l_right");
    auto inputDown = inputManager->getAnalog(path + "l_down");
    auto inputLeft = inputManager->getAnalog(path + "l_left");

    int16_t rawX = -inputLeft.value + inputRight.value;
    int16_t rawY = -inputUp.value + inputDown.value;

    if (!inputLeft.isRelative) rawX /= 16;
    if (!inputUp.isRelative) rawY /= 16;

    x = clamp<int16_t>(rawX, INT8_MIN, INT8_MAX);
    y = clamp<int16_t>(rawY, INT8_MIN, INT8_MAX);
}

};  // namespace peripherals