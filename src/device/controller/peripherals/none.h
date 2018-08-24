#pragma once
#include <string>
#include "abstract_device.h"
#include "device/device.h"

namespace peripherals {
struct None : public AbstractDevice {
    None(int port);
    uint8_t handle(uint8_t byte) override;
};
};  // namespace peripherals