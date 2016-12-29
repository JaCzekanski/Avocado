#pragma once
#include "device.h"
#include <deque>

namespace device {
namespace cdrom {

class CDROM : public Device {
    union CDROM_Status {
        struct {
            uint8_t index : 2;
            uint8_t xaFifoEmpty : 1;
            uint8_t parameterFifoEmpty : 1;  // triggered before writing first byte
            uint8_t parameterFifoFull : 1;   // triggered after writing 16 bytes
            uint8_t responseFifoEmpty : 1;   // triggered after reading last byte
            uint8_t dataFifoEmpty : 1;       // triggered after reading last byte
            uint8_t transmissionBusy : 1;
        };
        uint8_t _reg;

        CDROM_Status() : _reg(0x18) {}
    };

    CDROM_Status status;
    std::deque<uint8_t> CDROM_params;
    std::deque<uint8_t> CDROM_response;
    std::deque<uint8_t> CDROM_interrupt;

    void *_cpu = nullptr;
    int readSector = 0;

    void writeResponse(uint8_t byte) {
        CDROM_response.push_back(byte);
        status.responseFifoEmpty = 1;
    }

    uint8_t readParam() {
        uint8_t param = CDROM_params.front();
        CDROM_params.pop_front();

        status.parameterFifoEmpty = CDROM_params.empty();
        status.parameterFifoFull = 1;

        return param;
    }

   public:
    CDROM();
    void step();
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);

    void setCPU(void *cpu) { this->_cpu = cpu; }
};
}
}