#pragma once
#include <tuple>
#include "spu.h"

namespace spu {
    std::tuple<float, float> doReverb(SPU* spu, std::tuple<float, float> input);
}  // namespace spu