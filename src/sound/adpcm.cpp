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
    if (shift > 9) shift = 9;

    assert(filter <= 4);
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

template <int ch>
void interpolate(int16_t sample, std::vector<int16_t>& output) {
    ringbuf[ch][p[ch]++ & 0x1f] = sample;

    if (--sixstep[ch] == 0) {
        sixstep[ch] = 6;
        for (int table = 0; table < 7; table++) {
            output.push_back(doZigzag<ch>(p[ch], table));
        }
    }
}

enum class Channel { mono, left, right };

template <Channel channel>
std::vector<int16_t> decodePacket(uint8_t buffer[128], int32_t prevSample[2], bool sampleRate) {
    std::vector<int16_t> decoded;

    std::initializer_list<int> blocks;
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
        if (shift > 9) shift = 9;

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
                interpolate<0>(clamp_16bit(sample), decoded);

                // 18900Hz to 37800Hz
                // FIXME: Doubling samples isn't how you should interpolate it, right?
                if (sampleRate) interpolate<0>(clamp_16bit(sample), decoded);
            } else {
                interpolate<1>(clamp_16bit(sample), decoded);
                if (sampleRate) interpolate<0>(clamp_16bit(sample), decoded);
            }

            // Move previous samples forward
            prevSample[1] = prevSample[0];
            prevSample[0] = sample;
        }
    }

    return decoded;
}

std::pair<std::vector<int16_t>, std::vector<int16_t>> decodeXA(uint8_t buffer[128 * 18], cd::Codinginfo codinginfo) {
    static int32_t prevSampleLeft[2] = {};
    static int32_t prevSampleRight[2] = {};

    std::vector<int16_t> left;
    std::vector<int16_t> right;

    // Each sector contains of 18 128-byte portions
    for (int packet = 0; packet < 18; packet++) {
        if (codinginfo.stereo) {
            auto l = decodePacket<Channel::left>(buffer + packet * 128, prevSampleLeft, codinginfo.sampleRate);
            auto r = decodePacket<Channel::right>(buffer + packet * 128, prevSampleRight, codinginfo.sampleRate);

            left.insert(left.end(), l.begin(), l.end());
            right.insert(right.end(), r.begin(), r.end());
        } else {
            auto mono = decodePacket<Channel::mono>(buffer + packet * 128, prevSampleLeft, codinginfo.sampleRate);
            left.insert(left.end(), mono.begin(), mono.end());
            right.insert(right.end(), mono.begin(), mono.end());
        }
    }

    return std::make_pair(left, right);
}
}  // namespace ADPCM
