#pragma once
#include <vector>

namespace ADPCM {
std::vector<int16_t> decode(uint8_t* buffer, size_t sampleCount);  // 1 sample == 16 bytes
};                                                                 // namespace ADPCM