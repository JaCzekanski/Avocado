#include "spu.h"
#include "../mips.h"

namespace device {
namespace spu {
SPU::SPU() {}

void SPU::step() {}

uint8_t SPU::read(uint32_t address) {
    // 0x1f801c00 - voice base
    if (address >= 0 && address < 0x10 * VOICE_COUNT) {
        int voice = address / 0x10;
        int reg = address % 0x10;

        switch (reg) {
            case 0:
            case 1:
            case 2:
            case 3:
                return voices[voice].volume.read(reg);

            case 4:
            case 5:
                return voices[voice].sampleRate.read(reg - 4);

            case 6:
            case 7:
                return voices[voice].startAddress.read(reg - 6);

            case 8:
            case 9:
            case 10:
            case 11:
                return voices[voice].ADSR.read(reg - 8);

            case 12:
            case 13:
                return voices[voice].ADSRVolume.read(reg - 12);

            case 14:
            case 15:
                return voices[voice].repeatAddress.read(reg - 14);

            default:
                return 0;
        }
    }

    if (BASE_ADDRESS + address >= 0x1f801daa && BASE_ADDRESS + address <= 0x1f801dab)  // SPUCNT
    {
        return SPUCNT.read(address - (0x1f801daa - BASE_ADDRESS));
    }

    if (BASE_ADDRESS + address >= 0x1f801dae && BASE_ADDRESS + address <= 0x1f801daf)  // SPUSTAT
    {
        return SPUSTAT.read(address - (0x1f801dae - BASE_ADDRESS));
    }
    printf("UNHANDLED SPU READ AT 0x%08x\n", BASE_ADDRESS + address);

    return 0;
}

void SPU::write(uint32_t address, uint8_t data) {
    if (BASE_ADDRESS + address >= 0x1f801da8 && BASE_ADDRESS + address <= 0x1f801da9)  // SPU FIFO
    {
        // TODO
        return;
    }

    if (BASE_ADDRESS + address >= 0x1f801daa && BASE_ADDRESS + address <= 0x1f801dab)  // SPUCNT
    {
        SPUCNT.write(address - (0x1f801daa - BASE_ADDRESS), data);
        return;
    }

    if (BASE_ADDRESS + address >= 0x1f801dae && BASE_ADDRESS + address <= 0x1f801daf)  // SPUSTAT
    {
        SPUSTAT.write(address - (0x1f801dae - BASE_ADDRESS), data);
        return;
    }
    printf("UNHANDLED SPU WRITE AT 0x%08x: 0x%02x\n", BASE_ADDRESS + address, data);
}
}
}
