#pragma once
#include <vector>

namespace ADPCM {
enum Flag { End = 1 << 0, Repeat = 1 << 1, Start = 1 << 2 };
std::vector<int16_t> decode(uint8_t buffer[16], int32_t prevSample[2]);
};  // namespace ADPCM