#pragma once
#include <cstdint>

namespace timing {
constexpr uint64_t US_IN_SECOND = 1'000'000;
constexpr uint64_t CPU_CLOCK = 33'868'800;  // 44100 * 768
constexpr uint64_t GPU_CLOCK = 53'222'400;

// NTSC
const float DOTS_TOTAL = 3413.6f;
const int LINE_VBLANK_START_NTSC = 243;
const int LINES_TOTAL_NTSC = 263;
const float NTSC_FRAMERATE = (float)GPU_CLOCK / (DOTS_TOTAL * LINES_TOTAL_NTSC);


constexpr uint64_t usToCpuCycles(uint64_t us) { return us * CPU_CLOCK / US_IN_SECOND; }
}  // namespace timing
