#pragma once
#include <cstdint>

namespace gte {
struct Matrix {
    int16_t v11 = 0;
    int16_t v12 = 0;
    int16_t v13 = 0;

    int16_t v21 = 0;
    int16_t v22 = 0;
    int16_t v23 = 0;

    int16_t v31 = 0;
    int16_t v32 = 0;
    int16_t v33 = 0;
};

template <typename X, typename Y = X, typename Z = X>
struct Vector {
    union {
        X x, r;
    };

    union {
        Y y, g;
    };

    union {
        Z z, b;
    };
};

struct Color {
    int32_t r = 0;
    int32_t g = 0;
    int32_t b = 0;
};
};  // namespace gte