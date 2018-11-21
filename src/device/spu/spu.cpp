#include "spu.h"
#include <array>
#include <vector>
#include "device/cdrom.h"
#include "interpolation.h"
#include "reverb.h"
#include "sound/adpcm.h"
#include "system.h"
#include "utils/file.h"
#include "utils/math.h"

using namespace spu;

SPU::SPU(System* sys) : sys(sys) {
    ram.fill(0);
    audioBufferPos = 0;
}

void SPU::step(device::cdrom::CDROM* cdrom) {
    float sumLeft = 0, sumReverbLeft = 0;
    float sumRight = 0, sumReverbRight = 0;

	noise.doNoise(control.noiseFrequencyStep, control.noiseFrequencyShift);

    for (int v = 0; v < VOICE_COUNT; v++) {
        Voice& voice = voices[v];

        if (voice.state == Voice::State::Off) continue;

        if (voice.decodedSamples.empty()) {
            auto readAddress = voice.currentAddress._reg * 8;
            if (control.irqEnable && readAddress == irqAddress._reg * 8) {
                SPUSTAT._reg |= 1 << 6;
                sys->interrupt->trigger(interrupt::SPU);
            }
            voice.decodedSamples = ADPCM::decode(&ram[readAddress], voice.prevSample);
            voice.flagsParsed = false;
        }

        voice.processEnvelope();

        uint32_t step = voice.sampleRate._reg;
        if (voice.pitchModulation && v > 0 && !forcePitchModulationOff) {
            int32_t factor = static_cast<int32_t>(floatToInt(voices[v - 1].sample) + 0x8000);
            step = (step * factor) >> 15;
            step &= 0xffff;
        }
        if (step > 0x3fff) step = 0x4000;

        float sample;

		if (voice.mode == Voice::Mode::Noise) {
            sample = intToFloat(noise.getNoiseLevel());
		} else if (forceInterpolationOff) {
            sample = intToFloat(voice.decodedSamples[voice.counter.sample]);
        } else {
            sample = intToFloat(spu::interpolate(voice, voice.counter.sample, voice.counter.index));
        }
        sample *= intToFloat(voice.adsrVolume._reg);
        voice.sample = sample;

        sumLeft += sample * voice.volume.getLeft();
        sumRight += sample * voice.volume.getRight();

        if (voice.reverb) {
            sumReverbLeft += sample * voice.volume.getLeft();
            sumReverbRight += sample * voice.volume.getRight();
        }

        voice.counter._reg += step;
        if (voice.counter.sample >= 28) {
            // Overflow, parse next ADPCM block
            voice.counter.sample -= 28;
            voice.currentAddress._reg += 2;
            voice.prevDecodedSamples = voice.decodedSamples;
            voice.decodedSamples.clear();

            if (voice.loadRepeatAddress) {
                voice.loadRepeatAddress = false;
                voice.currentAddress = voice.repeatAddress;
            }
        }

        if (!voice.flagsParsed) {
            voice.parseFlags(ram[voice.currentAddress._reg * 8 + 1]);
        }
    }

    sumLeft *= mainVolume.getLeft();
    sumRight *= mainVolume.getRight();

    if (!forceReverbOff && control.masterReverb) {
        static float reverbLeft = 0.f, reverbRight = 0.f;
        static int reverbCounter = 0;
        if (reverbCounter++ % 2 == 0) {
            std::tie(reverbLeft, reverbRight) = doReverb(this, std::make_tuple(sumReverbLeft, sumReverbRight));
        }
        sumLeft += reverbLeft;
        sumRight += reverbRight;
    }

    // Mix with cd
    if (!cdrom->audio.first.empty()) {
        float left = intToFloat(cdrom->audio.first.front());
        float right = intToFloat(cdrom->audio.second.front());

        // TODO: Refactor to use ring buffer
        cdrom->audio.first.pop_front();
        cdrom->audio.second.pop_front();

        // 0x80 - full volume
        // 0xff - 2x volume
        float l_l = cdrom->volumeLeftToLeft / 128.f;
        float l_r = cdrom->volumeLeftToRight / 128.f;
        float r_l = cdrom->volumeRightToLeft / 128.f;
        float r_r = cdrom->volumeRightToRight / 128.f;

        sumLeft += left * l_l;
        sumRight += left * l_r;

        sumLeft += right * r_l;
        sumRight += right * r_r;
    }

    audioBuffer[audioBufferPos] = floatToInt(clamp(sumLeft, -1.f, 1.f));
    audioBuffer[audioBufferPos + 1] = floatToInt(clamp(sumRight, -1.f, 1.f));

    audioBufferPos += 2;
    if (audioBufferPos >= AUDIO_BUFFER_SIZE) {
        audioBufferPos = 0;
        bufferReady = true;
    }

    if (control.irqEnable && irqAddress._reg < 0x200) {
        static int cnt = 0;

        if (++cnt == 0x100) {
            cnt = 0;
            SPUSTAT._reg |= 1 << 6;
            sys->interrupt->trigger(interrupt::SPU);
        }
    }
}

uint8_t SPU::readVoice(uint32_t address) const {
    int voice = address / 0x10;
    int reg = address % 0x10;

    switch (reg) {
        case 0:
        case 1:
        case 2:
        case 3: return voices[voice].volume.read(reg);

        case 4:
        case 5: return voices[voice].sampleRate.read(reg - 4);

        case 6:
        case 7: return voices[voice].startAddress.read(reg - 6);

        case 8:
        case 9:
        case 10:
        case 11: return voices[voice].adsr.read(reg - 8);

        case 12:
        case 13: return voices[voice].adsrVolume.read(reg - 12);

        case 14:
        case 15: return voices[voice].repeatAddress.read(reg - 14);

        default: return 0;
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
            if (reg == 3 && ((voices[voice].volume.left & 0x8000) || (voices[voice].volume.right & 0x8000))) {
                printf("[SPU][WARN] Volume Sweep enabled for voice %d\n", voice);
            }
            return;

        case 4:
        case 5: voices[voice].sampleRate.write(reg - 4, data); return;

        case 6:
        case 7:
            voices[voice].counter._reg = 0;
            voices[voice].prevDecodedSamples.clear();  // TODO: Not sure is this is what real hardware does
            voices[voice].startAddress.write(reg - 6, data);
            return;

        case 8:
        case 9:
        case 10:
        case 11: voices[voice].adsr.write(reg - 8, data); return;

        case 12:
        case 13: voices[voice].adsrVolume.write(reg - 12, data); return;

        case 14:
        case 15:
            voices[voice].repeatAddress.write(reg - 14, data);
            voices[voice].ignoreLoadRepeatAddress = true;
            return;

        default: return;
    }
}

uint8_t SPU::read(uint32_t address) {
    address += BASE_ADDRESS;

    if (address >= 0x1f801c00 && address < 0x1f801c00 + 0x10 * VOICE_COUNT) {
        return readVoice(address - 0x1f801c00);
    }

    if (address >= 0x1f801d80 && address <= 0x1f801d83) {  // Main Volume L/R
        return mainVolume.read(address - 0x1f801d80);
    }

    if (address >= 0x1f801da6 && address <= 0x1f801da7) {  // Data address
        return dataAddress.read(address - 0x1f801da6);
    }

    if (address >= 0x1f801daa && address <= 0x1f801dab) {  // SPUCNT
        return control._byte[address - 0x1f801daa];
    }

    if (address >= 0x1f801db0 && address <= 0x1f801db3) {  // CD Volume L/R
        return cdVolume.read(address - 0x1f801db0);
    }

    if (address >= 0x1F801DB4 && address <= 0x1F801DB7) {  // External input Volume L/R
        return extVolume.read(address - 0x1F801DB4);
    }

    if (address >= 0x1F801D88 && address <= 0x1F801D8b) {  // Voice Key On
        // Shouldn't be read, but some games do, just return last value
        return _keyOn.read(address - 0x1F801D88);
    }

    if (address >= 0x1F801D8c && address <= 0x1F801D8f) {  // Voice Key Off
        // Shouldn't be read, but some games do
        return _keyOff.read(address - 0x1F801D8c);
    }

    if (address >= 0x1F801D90 && address <= 0x1F801D93) {  // Pitch modulation enable flags
        static Reg32 pitchModulation;

        if (address == 0x1F801D90) {
            pitchModulation._reg = 0;
            for (int v = 1; v < VOICE_COUNT; v++) {
                pitchModulation.setBit(v, voices[v].pitchModulation);
            }
        }

        return pitchModulation.read(address - 0x1F801D90);
    }

    if (address >= 0x1f801d9c && address <= 0x1f801d9f) {  // Voice ON/OFF status (ENDX)
        static Reg32 status;
        if (address == 0x1f801d9c) {
            status._reg = 0;
            for (int v = 0; v < VOICE_COUNT; v++) {
                status.setBit(v, voices[v].loopEnd);
            }
        }

        return status._byte[address - 0x1f801d9c];
    }

    if (address >= 0x1F801D94 && address <= 0x1F801D97) {  // Voice noise mode
        static Reg32 noiseEnabled;

        if (address == 0x1F801D94) {
            noiseEnabled._reg = 0;
            for (int v = 0; v < VOICE_COUNT; v++) {
                noiseEnabled.setBit(v, voices[v].mode == Voice::Mode::Noise);
            }
        }

        return noiseEnabled.read(address - 0x1F801D94);
    }

    if (address >= 0x1f801d98 && address <= 0x1f801d9b) {  // Voice Reverb
        static Reg32 reverb;
        if (address == 0x1f801d98) {
            reverb._reg = 0;
            for (int v = 0; v < VOICE_COUNT; v++) {
                reverb.setBit(v, voices[v].reverb);
            }
        }

        return reverb.read(address - 0x1f801d98);
    }

    if (address >= 0x1F801DA2 && address <= 0x1F801DA3) {  // Reverb Work area start
        // TODO: Breaks Doom if returning correct value, why ?
        return reverbBase.read(address - 0x1F801DA2);
    }

    if (address >= 0x1f801dac && address <= 0x1f801dad) {  // Data Transfer Control
        return dataTransferControl._byte[address - 0x1f801dac];
    }

    if (address >= 0x1f801dae && address <= 0x1f801daf) {  // SPUSTAT
        SPUSTAT._reg &= 0x0FC0;
        SPUSTAT._reg |= (control._reg & 0x3F);
        return SPUSTAT.read(address - 0x1f801dae);
    }

    if (address >= 0x1f801db8 && address <= 0x1f801dbb) {  // Current Main Volume L/R ?? (used by MGS)
        return mainVolume.read(address - 0x1f801db8);
    }

    if (address >= 0x1f801e00 && address < 0x1f801e00 + 4 * VOICE_COUNT) {  // Internal Voice volume (used by MGS)
        // TODO: Not sure if it is channel volume or processed sample volume
        int voice = (address - 0x1f801e00) / 4;
        int byte = (address - 0x1f801e00) % 4;
        return voices[voice].volume.read(byte);
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

    if (address >= 0x1f801d80 && address <= 0x1f801d83) {  // Main Volume L/R
        mainVolume.write(address - 0x1f801d80, data);
        return;
    }

    if (address >= 0x1f801d84 && address <= 0x1f801d87) {  // Reverb Volume L/R
        reverbVolume.write(address - 0x1f801d84, data);
        return;
    }

    if (address >= 0x1f801d88 && address <= 0x1f801d8b) {  // Voices Key On
        _keyOn.write(address - 0x1f801d88, data);
        if (address == 0x1f801d8b) {
            for (int i = 0; i < VOICE_COUNT; i++) {
                if (_keyOn.getBit(i)) voices[i].keyOn();
            }
        }
        return;
    }

    if (address >= 0x1f801d8c && address <= 0x1f801d8f) {  // Voices Key Off
        _keyOff.write(address - 0x1f801d8c, data);
        if (address == 0x1f801d8f) {
            for (int i = 0; i < VOICE_COUNT; i++) {
                if (_keyOff.getBit(i)) voices[i].keyOff();
            }
        }
        return;
    }

    if (address >= 0x1F801D90 && address <= 0x1F801D93) {  // Pitch modulation enable flags
        static Reg32 pitchModulation;

        pitchModulation.write(address - 0x1F801D90, data);
        if (address == 0x1F801D93) {
            for (int v = 1; v < VOICE_COUNT; v++) {
                voices[v].pitchModulation = pitchModulation.getBit(v);
            }
        }
        return;
    }

    if (address >= 0x1F801D94 && address <= 0x1F801D97) {  // Voice noise mode
        static Reg32 noiseEnabled;
        noiseEnabled.write(address - 0x1F801D94, data);

        if (address == 0x1F801D97) {
            for (int v = 0; v < VOICE_COUNT; v++) {
                voices[v].mode = noiseEnabled.getBit(v) ? Voice::Mode::Noise : Voice::Mode::ADSR;
            }
        }
        return;
    }

    if (address >= 0x1f801d98 && address <= 0x1f801d9b) {  // Voice Reverb
        static Reg32 reverb;
        reverb.write(address - 0x1f801d98, data);

        if (address == 0x1f801d9b) {
            for (int v = 0; v < VOICE_COUNT; v++) {
                voices[v].reverb = reverb.getBit(v);
            }
        }
        return;
    }

    if (address >= 0x1f801d9c && address <= 0x1f801d9f) {  // Voice ON/OFF status (ENDX)
        // Writes to this address should be ignored
        return;
    }

    if (address >= 0x1F801DA2 && address <= 0x1F801DA3) {  // Reverb Work area start
        reverbBase.write(address - 0x1F801DA2, data);
        if (address == 0x1F801DA3) {
            reverbCurrentAddress = reverbBase._reg * 8;
        }
        return;
    }

    if (address >= 0x1f801da4 && address <= 0x1f801da5) {  // IRQ Address
        irqAddress.write(address - 0x1f801da4, data);
        return;
    }

    if (address >= 0x1f801da6 && address <= 0x1f801da7) {  // Data address
        dataAddress.write(address - 0x1f801da6, data);
        currentDataAddress = dataAddress._reg * 8;

        sys->interrupt->trigger(interrupt::SPU);
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
        SPUSTAT._reg &= ~(1 << 6);
        control._byte[address - 0x1f801daa] = data;
        return;
    }

    if (address >= 0x1f801dac && address <= 0x1f801dad) {  // Data Transfer Control
        dataTransferControl._byte[address - 0x1f801dac] = data;
        return;
    }

    if (address >= 0x1f801db0 && address <= 0x1f801db3) {  // CD Volume L/R
        cdVolume.write(address - 0x1f801db0, data);
        return;
    }

    if (address >= 0x1F801DB4 && address <= 0x1F801DB7) {  // External input Volume L/R
        extVolume.write(address - 0x1F801DB4, data);
        return;
    }

    if (address >= 0x1F801DC0 && address <= 0x1F801DFF) {  // Reverb registers
        auto reg = (address - 0x1F801DC0) / 2;
        auto byte = (address - 0x1F801DC0) % 2;
        reverbRegisters[reg].write(byte, data);
        return;
    }

    printf("UNHANDLED SPU WRITE AT 0x%08x: 0x%02x\n", address, data);
}

void SPU::dumpRam() {
    std::vector<uint8_t> ram;
    ram.assign(this->ram.begin(), this->ram.end());
    putFileContents("spu.bin", ram);
}
