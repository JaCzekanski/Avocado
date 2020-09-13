#pragma once
#include <string>
#include "abstract_device.h"
#include "device/device.h"

namespace peripherals {
class FreecamController {
   public:
    void update(std::string path);
};
};  // namespace peripherals