#pragma once
#include <cstdint>

namespace gpu {
enum class SemiTransparency : uint32_t {
    Bby2plusFby2 = 0,  // B/2+F/2
    BplusF = 1,        // B+F
    BminusF = 2,       // B-F
    BplusFby4 = 3      // B+F/4
};
};