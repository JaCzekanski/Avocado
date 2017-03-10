#pragma once
#include "device.h"
#include <string>

namespace device {
class Dummy : public Device {
    std::string name;
    uint32_t baseAddress = 0;
    bool verbose = true;

   public:
    Dummy();
    Dummy(std::string name, uint32_t baseAddress = 0, bool verbose = true);
    void step();
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);
};
}
