#include "analog_controller.h"
#include "config.h"

namespace peripherals {
AnalogController::AnalogController() : AbstractDevice(Type::Analog), buttons(0) { 
    verbose = config["debug"]["log"]["controller"]; 
}

uint8_t AnalogController::handle(uint8_t byte) {
    if (state == 0) command = Command::None;
    if (verbose) printf("[CONTROLLER] state %d, input 0x%02x\n", state, byte);

    if (command == Command::Read && !analogEnabled) return handleRead(byte);
    if (command == Command::Read && analogEnabled) return handleReadAnalog(byte);
    if (command == Command::Configuration) return handleConfiguration(byte);
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
            if (byte == 'B') { // Read buttons
                command = Command::Read;
                state++;
                return ret;
            }
            if (byte == 'C') {  // Configuration mode
                command = Command::Configuration;
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

uint8_t AnalogController::handleRead(uint8_t byte) {
    switch (state) {
        case 2: state++; return 0x5a;
        case 3: state++; return ~buttons._byte[0];
        case 4: state = 0; return ~buttons._byte[1];

        default: return 0xff;
    }
}

uint8_t AnalogController::handleReadAnalog(uint8_t byte) {
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

uint8_t AnalogController::handleConfiguration(uint8_t byte) {
    switch (state) {
        case 2: state++; return 0x5a;
        case 3:
            configurationMode = (byte == 1);
            state++;
            return ~buttons._byte[0];
        case 4: state = analogEnabled ? state + 1 : 0; return ~buttons._byte[1];
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
        case 3: param = byte; state++; return 0;
        case 4: 
            if (param == 2) {
                ledEnabled = (byte == 1);
            }
            state++; return 0;
        case 5: state++; return 0;
        case 6: state++; return 0;
        case 7: state++; return 0;
        case 8: state = 0; return 0;

        default: return 0xff;
    }
}

uint8_t AnalogController::handleLedStatus(uint8_t byte) {
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
            analogEnabled = true; // TODO: Not completely valid, but should be good enough
            printf("[CONTROLLER] Unlock Rumble: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", a, b, c, d, e, f);
            return 0;

        default: return 0xff;
    }
}

uint8_t AnalogController::handleUnknown46(uint8_t byte) {
    static uint8_t param;
    switch (state) {
        case 2: state++; return 0x5a;
        case 3: param = byte; state++; return 0;
        case 4: state++; return 0;
        case 5: state++; return 1;
        case 6: state++; return param == 1 ? 1 : 2;
        case 7: state++; return param == 1 ? 1 : 0;
        case 8: state = 0; return param == 1 ? 0x14 : 0x0a;

        default: return 0xff;
    }
}

uint8_t AnalogController::handleUnknown47(uint8_t byte) {
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
        case 3: param = byte; state++; return 0;
        case 4: state++; return 0;
        case 5: state++; return 0;
        case 6: state++; return (param == 1) ? 0x07 : 0x04;
        case 7: state++; return 0;
        case 8: state = 0; return 0;

        default: return 0xff;
    }
}

};  // namespace peripherals