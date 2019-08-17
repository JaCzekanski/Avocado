#include "analog_controller.h"
#include "config.h"
#include "input/input_manager.h"

namespace peripherals {
AnalogController::AnalogController(int port) : DigitalController(Type::Analog, port) { verbose = config["debug"]["log"]["controller"]; }

uint8_t AnalogController::handle(uint8_t byte) {
    if (state == 0) command = Command::None;
    if (verbose >= 2) printf("[CONTROLLER%d] state %d, input 0x%02x\n", port, state, byte);

    if (command == Command::Read && !analogEnabled) return handleRead(byte);
    if (command == Command::Read && analogEnabled) return handleReadAnalog(byte);
    if (command == Command::EnterConfiguration) return handleEnterConfiguration(byte);
    if (command == Command::ExitConfiguration) return handleExitConfiguration(byte);
    if (command == Command::LedStatus) return handleLedStatus(byte);
    if (command == Command::SetLed) return handleSetLed(byte);
    if (command == Command::UnlockRumble) return handleUnlockRumble(byte);
    if (command == Command::Unknown_46) return handleUnknown46(byte);
    if (command == Command::Unknown_47) return handleUnknown47(byte);
    if (command == Command::Unknown_4c) return handleUnknown4c(byte);

    switch (state) {
        case 0:
            if (byte == 0x01) {
                state++;
                return 0xff;
            }
            return 0xff;

        case 1: {
            uint8_t ret = analogEnabled ? 0x73 : 0x41;
            if (configurationMode) ret = 0xf3;
            if (byte == 'B') {  // Read buttons
                command = Command::Read;
                state++;
                return ret;
            }
            if (byte == 'C') {  // Configuration mode
                if (!configurationMode)
                    command = Command::EnterConfiguration;
                else
                    command = Command::ExitConfiguration;
                state++;
                return ret;
            }
            if (byte == 0x44 && configurationMode) {  // Set LED status
                command = Command::SetLed;
                state++;
                return ret;
            }
            if (byte == 0x45 && configurationMode) {  // Get LED status
                command = Command::LedStatus;
                state++;
                return ret;
            }
            if (byte == 0x46 && configurationMode) {
                command = Command::Unknown_46;
                state++;
                return ret;
            }
            if (byte == 0x47 && configurationMode) {
                command = Command::Unknown_47;
                state++;
                return ret;
            }
            if (byte == 'M' && configurationMode) {  // Unlock rumble
                command = Command::UnlockRumble;
                state++;
                return ret;
            }
            if (byte == 0x4c && configurationMode) {
                command = Command::Unknown_4c;
                state++;
                return ret;
            }
            state = 0;
            return 0xff;
        }
        default: state = 0; return 0xff;
    }
}

uint8_t AnalogController::handleReadAnalog(uint8_t byte) {
    (void)byte;
    switch (state) {
        case 2: state++; return 0x5a;
        case 3: state++; return ~buttons._byte[0];
        case 4: state++; return ~buttons._byte[1];
        case 5: state++; return right.x;
        case 6: state++; return right.y;
        case 7: state++; return left.x;
        case 8: state = 0; return left.y;

        default: return 0xff;
    }
}

uint8_t AnalogController::handleEnterConfiguration(uint8_t byte) {
    switch (state) {
        case 2: state++; return 0x5a;
        case 3:
            configurationMode = (byte == 1);
            state++;
            return ~buttons._byte[0];
        case 4: state = analogEnabled ? state + 1 : 0; return ~buttons._byte[1];
        case 5: state++; return right.x;
        case 6: state++; return right.y;
        case 7: state++; return left.x;
        case 8: state = 0; return left.y;

        default: return 0xff;
    }
}
uint8_t AnalogController::handleExitConfiguration(uint8_t byte) {
    switch (state) {
        case 2: state++; return 0x5a;
        case 3:
            configurationMode = (byte == 1);
            state++;
            return 0;
        case 4: state++; return 0;
        case 5: state++; return 0;
        case 6: state++; return 0;
        case 7: state++; return 0;
        case 8: state = 0; return 0;

        default: return 0xff;
    }
}

uint8_t AnalogController::handleSetLed(uint8_t byte) {
    static uint8_t param;
    switch (state) {
        case 2: state++; return 0x5a;
        case 3:
            param = byte;
            state++;
            return 0;
        case 4:
            if (param == 2) {
                ledEnabled = (byte == 1);
            }
            state++;
            return 0;
        case 5: state++; return 0;
        case 6: state++; return 0;
        case 7: state++; return 0;
        case 8: state = 0; return 0;

        default: return 0xff;
    }
}

uint8_t AnalogController::handleLedStatus(uint8_t byte) {
    (void)byte;
    switch (state) {
        case 2: state++; return 0x5a;
        case 3: state++; return 1;
        case 4: state++; return 2;
        case 5: state++; return ledEnabled;
        case 6: state++; return 2;
        case 7: state++; return 1;
        case 8: state = 0; return 0;

        default: return 0xff;
    }
}

uint8_t AnalogController::handleUnlockRumble(uint8_t byte) {
    static uint8_t a, b, c, d, e, f;
    switch (state) {
        case 2: state++; return 0x5a;
        case 3:
            a = byte;
            state++;
            return 0;
        case 4:
            b = byte;
            state++;
            return 0;
        case 5:
            c = byte;
            state++;
            return 0;
        case 6:
            d = byte;
            state++;
            return 0;
        case 7:
            e = byte;
            state++;
            return 0;
        case 8:
            f = byte;
            state = 0;
            analogEnabled = true;  // TODO: Not completely valid, but should be good enough
            if (verbose >= 2)
                printf("[CONTROLLER%d] Unlock Rumble: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", port, a, b, c, d, e, f);
            return 0;

        default: return 0xff;
    }
}

uint8_t AnalogController::handleUnknown46(uint8_t byte) {
    static uint8_t param;
    switch (state) {
        case 2: state++; return 0x5a;
        case 3:
            param = byte;
            state++;
            return 0;
        case 4: state++; return 0;
        case 5: state++; return 1;
        case 6: state++; return param == 1 ? 1 : 2;
        case 7: state++; return param == 1 ? 1 : 0;
        case 8: state = 0; return param == 1 ? 0x14 : 0x0a;

        default: return 0xff;
    }
}

uint8_t AnalogController::handleUnknown47(uint8_t byte) {
    (void)byte;
    switch (state) {
        case 2: state++; return 0x5a;
        case 3: state++; return 0;
        case 4: state++; return 0;
        case 5: state++; return 2;
        case 6: state++; return 0;
        case 7: state++; return 1;
        case 8: state = 0; return 0;

        default: return 0xff;
    }
}

uint8_t AnalogController::handleUnknown4c(uint8_t byte) {
    static uint8_t param;
    switch (state) {
        case 2: state++; return 0x5a;
        case 3:
            param = byte;
            state++;
            return 0;
        case 4: state++; return 0;
        case 5: state++; return 0;
        case 6: state++; return (param == 1) ? 0x07 : 0x04;
        case 7: state++; return 0;
        case 8: state = 0; return 0;

        default: return 0xff;
    }
}

void AnalogController::update() {
    DigitalController::update();
    auto inputManager = InputManager::getInstance();
    if (inputManager == nullptr) return;

    static bool analogPressed = false;

    if (inputManager->getDigital(path + "analog")) {
        if (!analogPressed) {
            analogPressed = true;

            // Toggle analog mode
            analogEnabled = !analogEnabled;
            ledEnabled = analogEnabled;
            resetState();
            if (verbose >= 1) printf("[CONTROLLER%d] Analog mode %s\n", port, analogEnabled ? "enabled" : "disabled");
        }
    } else {
        analogPressed = false;
    }

    if (!analogEnabled) {
        buttons.l3 = 0;
        buttons.r3 = 0;
        return;
    }

    buttons.l3 = inputManager->getDigital(path + "l3");
    buttons.r3 = inputManager->getDigital(path + "r3");
    left.y = 0x80 + (-inputManager->getAnalog(path + "l_up").value + inputManager->getAnalog(path + "l_down").value);
    left.x = 0x80 + (-inputManager->getAnalog(path + "l_left").value + inputManager->getAnalog(path + "l_right").value);
    right.y = 0x80 + (-inputManager->getAnalog(path + "r_up").value + inputManager->getAnalog(path + "r_down").value);
    right.x = 0x80 + (-inputManager->getAnalog(path + "r_left").value + inputManager->getAnalog(path + "r_right").value);
}

};  // namespace peripherals