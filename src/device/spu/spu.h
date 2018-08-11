#pragma once
#include <array>
#include "device/device.h"
#include "regs.h"
#include "voice.h"

struct System;

namespace spu {
struct SPU {

    static const uint32_t BASE_ADDRESS = 0x1f801c00;
    static const int VOICE_COUNT = 24;
    static const int RAM_SIZE = 1024 * 512;
    static const size_t AUDIO_BUFFER_SIZE = 28 * 2 * 4;

    std::array<Voice, VOICE_COUNT> voices;

    Volume mainVolume;
    Volume cdVolume;
    Volume extVolume;
    Volume reverbVolume;

    Reg16 irqAddress;
    Reg16 dataAddress;
    uint32_t currentDataAddress;
    DataTransferControl dataTransferControl;
    Control control;
    Reg16 SPUSTAT;

    std::array<uint8_t, RAM_SIZE> ram;

    Reg16 reverbBase;
    std::array<Reg16, 32> reverbRegisters;

    bool bufferReady = false;
    size_t audioBufferPos;
    std::array<int16_t, AUDIO_BUFFER_SIZE> audioBuffer;

    System* sys;

    uint8_t readVoice(uint32_t address) const;
    void writeVoice(uint32_t address, uint8_t data);
    void voiceKeyOn(int i);
    void voiceKeyOff(int i);

    SPU(System* sys);
    void step();
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);

    void dumpRam();
};
}