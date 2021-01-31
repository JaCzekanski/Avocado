#pragma once
#include <cstdint>

namespace timing {
constexpr uint64_t US_IN_SECOND = 1'000'000;
constexpr uint64_t CPU_CLOCK = 33'868'800;  // 44100 * 768
constexpr uint64_t GPU_CLOCK = 53'222'400;

#define USE_EXACT_FPS
#ifdef USE_EXACT_FPS
const float CYCLES_PER_LINE_NTSC = 3413.6f;
const float CYCLES_PER_LINE_PAL = 3406.1f;
#else
const float CYCLES_PER_LINE_NTSC = 3372.7f;
const float CYCLES_PER_LINE_PAL = 3389;
#endif
const int LINES_TOTAL_NTSC = 263;
const int LINES_TOTAL_PAL = 314;

const double NTSC_FRAMERATE = (double)GPU_CLOCK / (CYCLES_PER_LINE_NTSC * LINES_TOTAL_NTSC);
const double PAL_FRAMERATE = (double)GPU_CLOCK / (CYCLES_PER_LINE_PAL * LINES_TOTAL_PAL);

constexpr uint64_t usToCpuCycles(uint64_t us) { return us * CPU_CLOCK / US_IN_SECOND; }
}  // namespace timing
