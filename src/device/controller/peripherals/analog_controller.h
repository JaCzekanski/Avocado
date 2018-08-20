#pragma once
#include <string>
#include "abstract_device.h"
#include "device/device.h"
#include "digital_controller.h"

namespace peripherals {
struct AnalogController : public AbstractDevice {
    enum class Command { None, Read, Configuration, SetLed, LedStatus, UnlockRumble, Unknown_46, Unknown_47, Unknown_4c };
    struct Stick {
        int8_t x;
        int8_t y;
        Stick() : x(0), y(0) {}
    };
    int verbose;
    DigitalController::ButtonState buttons;
    Stick left, right;
    Command command = Command::None;
    bool analogEnabled = false;
    bool ledEnabled = true;
    bool configurationMode = false;

    uint8_t handle(uint8_t byte) override;
    uint8_t handleRead(uint8_t byte);
    uint8_t handleReadAnalog(uint8_t byte);
    uint8_t handleConfiguration(uint8_t byte);
    uint8_t handleSetLed(uint8_t byte);
    uint8_t handleLedStatus(uint8_t byte);
    uint8_t handleUnlockRumble(uint8_t byte);
    uint8_t handleUnknown46(uint8_t byte);
    uint8_t handleUnknown47(uint8_t byte);
    uint8_t handleUnknown4c(uint8_t byte);
    AnalogController();
};
};  // namespace peripherals