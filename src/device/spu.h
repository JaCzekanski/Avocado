#pragma once
#include <deque>
#include <vector>
#include <array>
#include "device.h"
#include "utils/string.h"

struct SPU {
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
        Volume volume;
        Reg16 sampleRate;
        Reg16 startAddress;
        Reg32 ADSR;
        Reg16 ADSRVolume;
        Reg16 repeatAddress;

        Reg16 currentAddress;
        float subAddress;
        bool playing;

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
            playing = false;

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
    static const size_t AUDIO_BUFFER_SIZE = 28 * 2 * 100;

    bool bufferReady = false;

    Voice voices[VOICE_COUNT];

    Volume mainVolume;
    Volume cdVolume;
    Volume extVolume;
    Volume reverbVolume;

    Reg32 voiceChannelReverbMode;

    Reg16 irqAddress;
    Reg16 dataAddress;
    uint32_t currentDataAddress;
    DataTransferControl dataTransferControl;
    SpuControl control;
    Reg16 SPUSTAT;

    uint8_t ram[RAM_SIZE];

    size_t audioBufferPos;
    std::array<int16_t, AUDIO_BUFFER_SIZE> audioBuffer;

    uint8_t readVoice(uint32_t address) const;
    void writeVoice(uint32_t address, uint8_t data);
    void voiceKeyOn(int i);
    void voiceKeyOff(int i);

    SPU();
    void step();
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);

    void dumpRam();
};
