#include "math.h"

namespace gte {
Vector<int16_t> toVector(int16_t ir[4]) { return Vector<int16_t>(ir[1], ir[2], ir[3]); }
};  // namespace gte