#pragma once
#include "device/device.h"

namespace peripherals {
enum class Type { None, Digital, Analog, Mouse, MemoryCard };
struct AbstractDevice {
    Type type;
    int port;  // Physical port number
    int state = 0;

    AbstractDevice(Type type, int port);
    bool getAck();
    void resetState();

    // Handle serial communication with console
    virtual uint8_t handle(uint8_t byte) = 0;

    // Update key state
    virtual void update();
};
};  // namespace peripherals