#pragma once
#include "device/device.h"

namespace peripherals {
enum class Type {
    None, Digital, Analog, Mouse, MemoryCard
};
struct AbstractDevice {
    Type type;
    int state = 0;

    virtual uint8_t handle(uint8_t byte) = 0;
    
    bool getAck() {
        return state != 0;
    }
    virtual void resetState() {
        state = 0;
    }
    AbstractDevice(Type type) : type(type) {}
};
};  // namespace peripherals