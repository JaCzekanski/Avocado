#pragma once
#include "device.h"
#include <deque>

namespace device {
namespace cdrom {

class CDROM : public Device {
    int CDROM_index = 0;
    std::deque<uint8_t> CDROM_params;
    std::deque<uint8_t> CDROM_response;
    std::deque<uint8_t> CDROM_interrupt;

    void *_cpu = nullptr;
	int readSector = 0;

   public:
    CDROM();
    void step();
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);

    void setCPU(void *cpu) { this->_cpu = cpu; }
};
}
}