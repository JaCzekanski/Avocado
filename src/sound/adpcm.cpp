#include "adpcm.h"
#include <cassert>
#include "tables.h"

namespace ADPCM {
int filterTablePos[5] = {0, 60, 115, 98, 122};
int filterTableNeg[5] = {0, 0, -52, -55, -60};

int16_t clamp_16bit(int32_t sample) {
    if (sample > 0x7fff) return 0x7fff;
    if (sample < -0x8000) return -0x8000;

    return (int16_t)sample;
}

std::vector<int16_t> decode(uint8_t buffer[16], int32_t prevSample[2]) {
    std::vector<int16_t> decoded;
    decoded.reserve(28);

    // Read ADPCM header
    auto shift = buffer[0] & 0x0f;
    auto filter = (buffer[0] & 0x70) >> 4;  // 0x40 for xa adpcm
    if (shift > 12) shift = 9;

    if (filter > 4) filter = 4;  // TODO: Not sure, check behaviour on real HW

    auto filterPos = filterTablePos[filter];
    auto filterNeg = filterTableNeg[filter];

    for (auto n = 0; n < 28; n++) {
        // Read currently decoded nibble
        int16_t nibble = buffer[2 + n / 2];
        if (n % 2 == 0) {
            nibble = (nibble & 0x0f);
        } else {
            nibble = (nibble & 0xf0) >> 4;
        }

        // Extend 4bit sample to 16bit
        int32_t sample = (int32_t)(int16_t)(nibble << 12);

        // Shift right by value in header
        sample >>= shift;

        // Mix previous samples
        sample += (prevSample[0] * filterPos + prevSample[1] * filterNeg + 32) / 64;

        // clamp to -0x8000 +0x7fff
        decoded.push_back(clamp_16bit(sample));

        // Move previous samples forward
        prevSample[1] = prevSample[0];
        prevSample[0] = sample;
    }

    return decoded;
}

// Separate buffers and counters for left and right channels
// TODO: Come up with better solution. Are two buffers really necessary?
int16_t ringbuf[2][0x20] = {};
int p[2] = {};
int sixstep[2] = {6, 6};

template <int ch>
int16_t doZigzag(int p, int table) {
    int32_t sum = 0;
    for (int i = 1; i < 29; i++) {
        sum += (ringbuf[ch][(p - i) & 0x1f] * zigzagTables[table][i]) / 0x8000;
    }
    return clamp_16bit(sum);
}

// sampleRate == false - 37800Hz
// sampleRate == true  - 18900Hz - double output samples
template <int ch>
void interpolate(int16_t sample, std::vector<int16_t>& output, bool sampleRate = false) {
    ringbuf[ch][p[ch]++ & 0x1f] = sample;

    if (--sixstep[ch] == 0) {
        sixstep[ch] = 6;
        for (int table = 0; table < 7; table++) {
            int16_t v = doZigzag<ch>(p[ch], table);
            output.push_back(v);
            if (sampleRate) output.push_back(v);
        }
    }
}

enum class Channel { mono, left, right };

template <Channel channel>
std::vector<int16_t> decodePacket(uint8_t buffer[128], int32_t prevSample[2], bool sampleRate) {
    std::vector<int16_t> decoded;

    std::vector<int> blocks;
    if (channel == Channel::mono) {
        blocks = {0, 1, 2, 3, 4, 5, 6, 7};
    } else if (channel == Channel::left) {
        blocks = {0, 2, 4, 6};
    } else if (channel == Channel::right) {
        blocks = {1, 3, 5, 7};
    }

    for (auto block : blocks) {
        // Read ADPCM header
        auto shift = buffer[4 + block] & 0x0f;
        auto filter = (buffer[4 + block] & 0x30) >> 4;
        if (shift > 12) shift = 9;

        auto filterPos = filterTablePos[filter];
        auto filterNeg = filterTableNeg[filter];

        for (auto n = 0; n < 28; n++) {
            // Read currently decoded nibble
            uint32_t word = buffer[0x10 + n * 4 + 0];
            word |= buffer[0x10 + n * 4 + 1] << 8;
            word |= buffer[0x10 + n * 4 + 2] << 16;
            word |= buffer[0x10 + n * 4 + 3] << 24;

            int16_t nibble = (word >> (block * 4)) & 0xf;

            // Extend 4bit sample to 16bit
            int32_t sample = (int32_t)(int16_t)(nibble << 12);

            // Shift right by value in header
            sample >>= shift;

            // Mix previous samples
            sample += (prevSample[0] * filterPos + prevSample[1] * filterNeg + 32) / 64;

            // clamp to -0x8000 +0x7fff
            // Intepolate 37800Hz to 44100Hz
            if (channel == Channel::mono || channel == Channel::left) {
                interpolate<0>(clamp_16bit(sample), decoded, sampleRate);
            } else {
                interpolate<1>(clamp_16bit(sample), decoded, sampleRate);
            }

            // Move previous samples forward
            prevSample[1] = prevSample[0];
            prevSample[0] = sample;
        }
    }

    return decoded;
}

std::vector<std::pair<int16_t, int16_t>> decodeXA(uint8_t buffer[128 * 18], cd::Codinginfo codinginfo) {
    static int32_t prevSampleLeft[2] = {};
    static int32_t prevSampleRight[2] = {};

    std::vector<std::pair<int16_t, int16_t>> frame;

    // Each sector contains of 18 128-byte portions
    for (int packet = 0; packet < 18; packet++) {
        if (codinginfo.stereo) {
            auto l = decodePacket<Channel::left>(buffer + packet * 128, prevSampleLeft, codinginfo.sampleRate);
            auto r = decodePacket<Channel::right>(buffer + packet * 128, prevSampleRight, codinginfo.sampleRate);

            for (int i = 0; i < l.size(); i++) {
                frame.emplace_back(l[i], r[i]);
            }
        } else {
            auto mono = decodePacket<Channel::mono>(buffer + packet * 128, prevSampleLeft, codinginfo.sampleRate);
            for (auto sample : mono) {
                frame.emplace_back(sample, sample);
            }
        }
    }

    return frame;
}
}  // namespace ADPCM
