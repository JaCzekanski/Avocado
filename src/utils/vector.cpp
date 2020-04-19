#include "vector.h"
#include <cmath>

float vec2::length() const { return sqrtf(x * x + y * y); }

vec2 vec2::normalize(const vec2& v) {
    float length = v.length();
    return {v.x / length, v.y / length};
}