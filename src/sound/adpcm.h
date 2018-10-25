#pragma once
#include <vector>
#include "utils/cd.h"

namespace ADPCM {
enum Flag { End = 1 << 0, Repeat = 1 << 1, Start = 1 << 2 };
std::vector<int16_t> decode(uint8_t buffer[16], int32_t prevSample[2]);
std::pair<std::vector<int16_t>, std::vector<int16_t>> decodeXA(uint8_t buffer[128 * 18], cd::Codinginfo codinginfo);
};  // namespace ADPCM