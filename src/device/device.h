#pragma once
#include <stdint.h>

namespace device {
	typedef uint32_t Bit;
class Device {
   public:
    virtual ~Device(){};
    virtual void step() = 0;
    virtual uint8_t read(uint32_t address) = 0;
    virtual void write(uint32_t address, uint8_t data) = 0;
};
}