#include "psx_color.h"

RGB operator*(const RGB& lhs, const float rhs) {
    RGB c;
    c.r = (uint8_t)(lhs.r * rhs);
    c.g = (uint8_t)(lhs.g * rhs);
    c.b = (uint8_t)(lhs.b * rhs);
    return c;
}

RGB operator+(const RGB& lhs, const RGB& rhs) {
    RGB c;
    c.r = lhs.r + rhs.r;
    c.g = lhs.g + rhs.g;
    c.b = lhs.b + rhs.b;
    return c;
}