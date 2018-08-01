#pragma once
#include <vector>

namespace ADPCM {
std::vector<int16_t> decode(uint8_t buffer[16], int32_t prevSample[2]);
};  // namespace ADPCM