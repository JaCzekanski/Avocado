#pragma once
#include <deque>
#include "device.h"
#include "utils/string.h"

struct SPU {
    struct Voice {
        Reg32 volume;
        Reg16 sampleRate;
        Reg16 startAddress;
        Reg32 ADSR;
        Reg16 ADSRVolume;
        Reg16 repeatAddress;
    };

    union DataTransferControl {
        struct {
            uint16_t : 1;
            uint16_t transferType : 3;
            uint16_t : 12;
        };
        uint16_t _reg;
        uint8_t _byte[2];

        DataTransferControl() : _reg(0) {}
    };

    union SpuControl {
        struct {
            uint16_t cdEnable : 1;
            uint16_t externalEnable : 1;
            uint16_t cdReverb : 1;
            uint16_t externalReverb : 1;
            uint16_t transferType : 2;  // 0 - stop, 1 - Manual, 2 - DMA Write, 3 - DMA Read
            uint16_t irqEnable : 1;
            uint16_t masterReverb : 1;
            uint16_t noiseFrequencyStep : 2;  // 4,5,6,7
            uint16_t noiseFrequencyShift : 4;
            uint16_t mute : 1;  // 0 - mute, 1 - unmute
            uint16_t spuEnable : 1;
        };
        uint16_t _reg;
        uint8_t _byte[2];

        SpuControl() : _reg(0) {}
    };

    static const uint32_t BASE_ADDRESS = 0x1f801c00;
    static const int VOICE_COUNT = 24;
    static const int RAM_SIZE = 1024 * 512;

    Voice voices[VOICE_COUNT];

    Reg32 mainVolume;
    Reg32 cdVolume;
    Reg32 extVolume;
    Reg32 reverbVolume;
    Reg32 voiceKeyOn;
    Reg32 voiceKeyOff;

    Reg32 voiceChannelReverbMode;

    Reg16 irqAddress;
    Reg16 dataAddress;
    uint32_t currentDataAddress;
    DataTransferControl dataTransferControl;

    SpuControl control;

    uint8_t ram[1024 * 512];

    uint8_t readVoice(uint32_t address) const;
    void writeVoice(uint32_t address, uint8_t data);

    Reg16 SPUSTAT;

    SPU();
    void step();
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);

    void dumpRam();
};
