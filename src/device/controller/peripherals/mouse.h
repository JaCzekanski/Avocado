#pragma once
#include <string>
#include "abstract_device.h"
#include "device/device.h"

namespace peripherals {
class Mouse : public AbstractDevice {
    std::string path;
    bool left = false, right = false;
    int8_t x = 0, y = 0;

   public:
    Mouse(int port);
    uint8_t handle(uint8_t byte) override;
    void update() override;
};
};  // namespace peripherals