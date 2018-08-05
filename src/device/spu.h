#pragma once
#include <array>
#include <deque>
#include <vector>
#include "device.h"
#include "utils/string.h"

struct System;

struct SPU {
    union ADSR {
        struct {
            uint32_t sustainLevel : 4;

            // No decay step, always -8
            uint32_t decayShift : 4;
            // No decay direction, always decrease
            // No decay mode, always Exponential

            uint32_t attackStep : 2;
            uint32_t attackShift : 5;
            // No attack direction, always increase
            uint32_t attackMode : 1;

            // No release step, always -8
            uint32_t releaseShift : 5;
            // No release direction, always decrease
            uint32_t releaseMode : 1;

            uint32_t sustainStep : 2;
            uint32_t sustainShift : 5;
            uint32_t : 1;
            uint32_t sustainDirection : 1;  // 0 - increase, 1 - decrease
            uint32_t sustainMode : 1;       // 0 - linear, 1 - exponential
        };
        uint32_t _reg;
        uint8_t _byte[4];

        ADSR() : _reg(0) {}

        void write(int n, uint8_t v) {
            if (n >= 4) return;
            _byte[n] = v;
        }

        uint8_t read(int n) const {
            if (n >= 4) return 0;
            return _byte[n];
        }
    };

    union Volume {
        struct {
            uint16_t left;
            uint16_t right;
        };
        uint32_t _reg;
        uint8_t _byte[4];

        Volume() : left(0), right(0) {}

        void write(int n, uint8_t v) {
            if (n >= 4) return;
            _byte[n] = v;
        }

        uint8_t read(int n) const {
            if (n >= 4) return 0;
            return _byte[n];
        }

        float getLeft() {
            if (left & 0x8000) return 1.f;  // TODO: Implement ADSR
            return ((int16_t)(left << 1) / (float)0x7fff);
        }

        float getRight() {
            if (right & 0x8000) return 1.f;  // TODO: Implement ADSR
            return ((int16_t)(right << 1) / (float)0x7fff);
        }
    };
    struct Voice {
        enum class State { Attack, Decay, Sustain, Release, Off };
        enum class Mode { ADSR, Noise };
        Volume volume;
        Reg16 sampleRate;
        Reg16 startAddress;
        ADSR ADSR;
        Reg16 ADSRVolume;
        Reg16 repeatAddress;

        Reg16 currentAddress;
        float subAddress;
        State state;
        Mode mode;
        bool pitchModulation;
        bool reverb;

        bool loopEnd;

        // ADPCM decoding
        int32_t prevSample[2];
        std::vector<int16_t> decodedSamples;

        Voice() {
            volume._reg = 0;
            sampleRate._reg = 0;
            startAddress._reg = 0;
            ADSR._reg = 0;
            ADSRVolume._reg = 0;
            repeatAddress._reg = 0;
            currentAddress._reg = 0;
            subAddress = 0;
            state = State::Off;
            loopEnd = false;
            mode = Mode::ADSR;
            pitchModulation = false;
            reverb = false;

            prevSample[0] = prevSample[1] = 0;
        }
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
    static const size_t AUDIO_BUFFER_SIZE = 28 * 2 * 10;

    bool bufferReady = false;

    Voice voices[VOICE_COUNT];

    Volume mainVolume;
    Volume cdVolume;
    Volume extVolume;
    Volume reverbVolume;

    Reg16 irqAddress;
    Reg16 dataAddress;
    uint32_t currentDataAddress;
    DataTransferControl dataTransferControl;
    SpuControl control;
    Reg16 SPUSTAT;

    uint8_t ram[RAM_SIZE];

    Reg16 reverbBase;
    Reg16 reverbRegisters[32];

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
