#pragma once
#include <vector>
#include "utils/cd.h"

namespace ADPCM {
enum Flag {
    LoopEnd = 1 << 0,  // Jump to repeat address after this block
                       // 1 - Copy repeatAddress to currentAddress AFTER this block
                       //     set ENDX (TODO: Immediately or after this block?)
                       // 0 - Nothing

    Repeat = 1 << 1,  // Takes an effect only with LoopEnd bit set.
                      // 1 - Loop normally
                      // 0 - Loop and force Release

    LoopStart = 1 << 2,  // Mark current address as the beginning of repeat
                         // 1 - Load currentAddress to repeatAddress
                         // 0 - Nothing
};
std::vector<int16_t> decode(uint8_t buffer[16], int32_t prevSample[2]);
std::pair<std::vector<int16_t>, std::vector<int16_t>> decodeXA(uint8_t buffer[128 * 18], cd::Codinginfo codinginfo);
};  // namespace ADPCM