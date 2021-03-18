#include "cop0.h"

std::pair<uint32_t, bool> COP0::read(int reg) {
    uint32_t value = 0;
    bool throwException = false;
    switch (reg) {
        case 3: value = bpc; break;
        case 5: value = bda; break;
        case 6: value = tar; break;
        case 7: value = dcic._reg; break;
        case 8: value = bada; break;
        case 9: value = bdam; break;
        case 11: value = bpcm; break;
        case 12: value = status._reg; break;
        case 13: value = cause._reg; break;
        case 14: value = epc; break;
        case 15: value = prid; break;
        default: throwException = true; break;
    }

    return std::make_pair(value, throwException);
}

void COP0::write(int reg, uint32_t value) {
    switch (reg) {
        case 3: bpc = value; break;
        case 5: bda = value; break;
        case 7: dcic._reg = value; break;
        case 9: bdam = value; break;
        case 11: bpcm = value; break;
        case 12:
            if (value & (1 << 17)) {
                // TODO: Handle SwC
            }
            status._reg = value;
            break;
        case 13:
            cause.interruptPending &= 3;
            cause.interruptPending |= (value & 0x300) >> 8;
            break;
        case 14: epc = value; break;
        default: break;
    }
}

void COP0::returnFromException() {
    status.interruptEnable = status.previousInterruptEnable;
    status.mode = status.previousMode;

    status.previousInterruptEnable = status.oldInterruptEnable;
    status.previousMode = status.oldMode;
}
