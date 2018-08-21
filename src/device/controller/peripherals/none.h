#pragma once
#include <string>
#include "abstract_device.h"
#include "device/device.h"

namespace peripherals {
struct None : public AbstractDevice {
    uint8_t handle(uint8_t byte) override;
    None();
};
};  // namespace peripherals