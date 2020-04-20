#include "texture_utils.h"

ColorDepth bitsToDepth(int bits) {
    switch (bits) {
        case 4: return ColorDepth::BIT_4;
        case 8: return ColorDepth::BIT_8;
        case 16: return ColorDepth::BIT_16;
        default: return ColorDepth::NONE;
    }
}