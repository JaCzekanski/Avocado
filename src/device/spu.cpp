#include "spu.h"
#include "../mips.h"
#include <cassert>

namespace device {
namespace spu {

SPU::SPU() {}

void SPU::step() {}

uint8_t SPU::readVoice(uint32_t address) const {
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

void SPU::writeVoice(uint32_t address, uint8_t data) {
    int voice = address / 0x10;
    int reg = address % 0x10;

    switch (reg) {
        case 0:
        case 1:
        case 2:
        case 3:
            voices[voice].volume.write(reg, data);
            return;

        case 4:
        case 5:
            voices[voice].sampleRate.write(reg - 4, data);
            return;

        case 6:
        case 7:
            voices[voice].startAddress.write(reg - 6, data);
            return;

        case 8:
        case 9:
        case 10:
        case 11:
            voices[voice].ADSR.write(reg - 8, data);
            return;

        case 12:
        case 13:
            voices[voice].ADSRVolume.write(reg - 12, data);
            return;

        case 14:
        case 15:
            voices[voice].repeatAddress.write(reg - 14, data);
            return;

        default:
            return;
    }
}

uint8_t SPU::read(uint32_t address) {
    address += BASE_ADDRESS;

    if (address >= 0x1f801c00 && address < 0x1f801c00 + 0x10 * VOICE_COUNT) {
        return readVoice(address - 0x1f801c00);
    }

    if (address >= 0x1f801da6 && address <= 0x1f801da7) {  // Data address
        return dataAddress.read(address - 0x1f801da6);
    }

    if (address >= 0x1f801daa && address <= 0x1f801dab) {  // SPUCNT
        printf("SPUCNT READ 0x%04x\n", SPUCNT._reg);
        return SPUCNT.read(address - 0x1f801daa);
    }

    if (address >= 0x1f801dae && address <= 0x1f801daf) {  // SPUSTAT
        SPUSTAT._reg = SPUCNT._reg & 0x3F;
        printf("SPUSTAT READ 0x%04x\n", SPUSTAT._reg);
        return SPUSTAT.read(address - 0x1f801dae);
    }

    printf("UNHANDLED SPU READ AT 0x%08x\n", address);

    return 0;
}

void SPU::write(uint32_t address, uint8_t data) {
    address += BASE_ADDRESS;

    if (address >= 0x1f801c00 && address < 0x1f801c00 + 0x10 * VOICE_COUNT) {
        writeVoice(address - 0x1f801c00, data);
        return;
    }

    if (address >= 0x1f801da4 && address <= 0x1f801da5) {  // IRQ Address
        irqAddress.write(address - 0x1f801da4, data);
        printf("IRQ ADDRESS: 0x%08x\n", irqAddress._reg * 8);
        return;
    }

    if (address >= 0x1f801da6 && address <= 0x1f801da7) {  // Data address
        dataAddress.write(address - 0x1f801da6, data);
        currentDataAddress = dataAddress._reg * 8;
        return;
    }

    if (address >= 0x1f801da8 && address <= 0x1f801da9) {  // Data FIFO
        if (currentDataAddress >= RAM_SIZE) {
            currentDataAddress %= RAM_SIZE;
        }
        ram[currentDataAddress++] = data;
        return;
    }

    if (address >= 0x1f801daa && address <= 0x1f801dab) {  // SPUCNT
        SPUCNT.write(address - 0x1f801daa, data);
        return;
    }

    if (address >= 0x1f801dac && address <= 0x1f801dad) {  // Data Transfer Control
        dataTransferControl._byte[address - 0x1f801dac] = data;
        return;
    }

    if (address >= 0x1f801dae && address <= 0x1f801daf) {  // SPUSTAT
        SPUSTAT.write(address - 0x1f801dae, data);
        return;
    }
    printf("UNHANDLED SPU WRITE AT 0x%08x: 0x%02x\n", address, data);
}
}
}
