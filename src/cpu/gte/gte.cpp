#include "gte.h"

uint32_t GTE::read(uint8_t n) {
    uint32_t ret = [this](uint8_t n) -> uint32_t {
        switch (n) {
            case 1: return (int32_t)v[0].z;
            case 2: return ((uint16_t)v[1].y << 16) | (uint16_t)v[1].x;
            case 3: return (int32_t)v[1].z;
            case 4: return ((uint16_t)v[2].y << 16) | (uint16_t)v[2].x;
            case 5: return (int32_t)v[2].z;
            case 6: return rgbc._reg;
            case 7: return otz;

            case 8: return (int32_t)ir[0];
            case 9: return (int32_t)ir[1];
            case 10: return (int32_t)ir[2];
            case 11: return (int32_t)ir[3];
            case 12: return ((uint16_t)s[0].y << 16) | (uint16_t)s[0].x;
            case 13: return ((uint16_t)s[1].y << 16) | (uint16_t)s[1].x;
            case 14:
            case 15: return ((uint16_t)s[2].y << 16) | (uint16_t)s[2].x;

            case 16: return (uint16_t)s[0].z;
            case 17: return (uint16_t)s[1].z;
            case 18: return (uint16_t)s[2].z;
            case 19: return (uint16_t)s[3].z;
            case 20: return rgb[0]._reg;
            case 21: return rgb[1]._reg;
            case 22: return rgb[2]._reg;
            case 23: return res1;

            case 24: return mac[0];
            case 25: return mac[1];
            case 26: return mac[2];
            case 27: return mac[3];
            case 28:
            case 29:
                irgb = clip(ir[1] / 0x80, 0x1f, 0x00);
                irgb |= clip(ir[2] / 0x80, 0x1f, 0x00) << 5;
                irgb |= clip(ir[3] / 0x80, 0x1f, 0x00) << 10;
                return irgb;
            case 30: return lzcs;
            case 31: return lzcr;

            case 32: return ((uint16_t)rt.v12 << 16) | (uint16_t)rt.v11;
            case 33: return ((uint16_t)rt.v21 << 16) | (uint16_t)rt.v13;
            case 34: return ((uint16_t)rt.v23 << 16) | (uint16_t)rt.v22;
            case 35: return ((uint16_t)rt.v32 << 16) | (uint16_t)rt.v31;
            case 36: return (int32_t)rt.v33;
            case 37: return tr.x;
            case 38: return tr.y;
            case 39: return tr.z;

            case 40: return ((uint16_t)l.v12 << 16) | (uint16_t)l.v11;
            case 41: return ((uint16_t)l.v21 << 16) | (uint16_t)l.v13;
            case 42: return ((uint16_t)l.v23 << 16) | (uint16_t)l.v22;
            case 43: return ((uint16_t)l.v32 << 16) | (uint16_t)l.v31;
            case 44: return (int32_t)l.v33;
            case 45: return bk.r;
            case 46: return bk.g;
            case 47: return bk.b;

            case 48: return ((uint16_t)lr.v12 << 16) | (uint16_t)lr.v11;
            case 49: return ((uint16_t)lr.v21 << 16) | (uint16_t)lr.v13;
            case 50: return ((uint16_t)lr.v23 << 16) | (uint16_t)lr.v22;
            case 51: return ((uint16_t)lr.v32 << 16) | (uint16_t)lr.v31;
            case 52: return (int32_t)lr.v33;
            case 53: return fc.r;
            case 54: return fc.g;
            case 55: return fc.b;

            case 56: return of[0];
            case 57: return of[1];
            case 58:
                return (int32_t)(int16_t)h;  // gte_bug: H is sign extended on read
            case 59: return (int32_t)dqa;
            case 60: return dqb;
            case 61:
                return (int32_t)(int16_t)zsf3;  // gte_bug?: sign extended
            case 62:
                return (int32_t)(int16_t)zsf4;  // gte_bug?: sign extended
            case 63: flag.calculate(); return flag.reg;
            default: return 0;
        }
    }(n);

    log.push_back({GTE_ENTRY::MODE::read, n, ret});
    return ret;
}

void GTE::write(uint8_t n, uint32_t d) {
    log.push_back({GTE_ENTRY::MODE::write, n, d});
    switch (n) {
        case 0:
            v[0].y = d >> 16;
            v[0].x = d;
            break;
        case 1: v[0].z = d; break;
        case 2:
            v[1].y = d >> 16;
            v[1].x = d;
            break;
        case 3: v[1].z = d; break;
        case 4:
            v[2].y = d >> 16;
            v[2].x = d;
            break;
        case 5: v[2].z = d; break;
        case 6: rgbc._reg = d; break;
        case 7: otz = d; break;

        case 8: ir[0] = d; break;
        case 9: ir[1] = d; break;
        case 10: ir[2] = d; break;
        case 11: ir[3] = d; break;
        case 12:
            s[0].y = d >> 16;
            s[0].x = d;
            break;
        case 13:
            s[1].y = d >> 16;
            s[1].x = d;
            break;
        case 14:
            s[2].y = d >> 16;
            s[2].x = d;
            break;
        case 15:
            s[0].x = s[1].x;
            s[0].y = s[1].y;

            s[1].x = s[2].x;
            s[1].y = s[2].y;

            s[2].y = d >> 16;
            s[2].x = d;
            break;

        case 16: s[0].z = d; break;
        case 17: s[1].z = d; break;
        case 18: s[2].z = d; break;
        case 19: s[3].z = d; break;
        case 20: rgb[0]._reg = d; break;
        case 21: rgb[1]._reg = d; break;
        case 22: rgb[2]._reg = d; break;
        case 23: res1 = d; break;

        case 24: mac[0] = d; break;
        case 25: mac[1] = d; break;
        case 26: mac[2] = d; break;
        case 27: mac[3] = d; break;
        case 28:
            irgb = d & 0x00007fff;
            ir[1] = (d & 0x1f) * 0x80;
            ir[2] = ((d >> 5) & 0x1f) * 0x80;
            ir[3] = ((d >> 10) & 0x1f) * 0x80;
            break;
        case 29: break;
        case 30:
            lzcs = d;
            lzcr = countLeadingZeroes(lzcs);
            break;
        case 31: break;

        case 32:
            rt.v12 = d >> 16;
            rt.v11 = d;
            break;
        case 33:
            rt.v21 = d >> 16;
            rt.v13 = d;
            break;
        case 34:
            rt.v23 = d >> 16;
            rt.v22 = d;
            break;
        case 35:
            rt.v32 = d >> 16;
            rt.v31 = d;
            break;
        case 36: rt.v33 = d; break;
        case 37: tr.x = d; break;
        case 38: tr.y = d; break;
        case 39: tr.z = d; break;

        case 40:
            l.v12 = d >> 16;
            l.v11 = d;
            break;
        case 41:
            l.v21 = d >> 16;
            l.v13 = d;
            break;
        case 42:
            l.v23 = d >> 16;
            l.v22 = d;
            break;
        case 43:
            l.v32 = d >> 16;
            l.v31 = d;
            break;
        case 44: l.v33 = d; break;
        case 45: bk.r = d; break;
        case 46: bk.g = d; break;
        case 47: bk.b = d; break;

        case 48:
            lr.v12 = d >> 16;
            lr.v11 = d;
            break;
        case 49:
            lr.v21 = d >> 16;
            lr.v13 = d;
            break;
        case 50:
            lr.v23 = d >> 16;
            lr.v22 = d;
            break;
        case 51:
            lr.v32 = d >> 16;
            lr.v31 = d;
            break;
        case 52: lr.v33 = d; break;
        case 53: fc.r = d; break;
        case 54: fc.g = d; break;
        case 55: fc.b = d; break;

        case 56: of[0] = d; break;
        case 57: of[1] = d; break;
        case 58: h = d; break;
        case 59: dqa = d; break;
        case 60: dqb = d; break;
        case 61: zsf3 = d; break;
        case 62: zsf4 = d; break;
        case 63: flag.reg = d & 0x7FFFF000; break;
        default: return;
    }
}

bool GTE::command(gte::Command& cmd) {
    flag.reg = 0;
    this->sf = cmd.sf;
    this->lm = cmd.lm;

    switch (cmd.cmd) {
        case 0x01: rtps(0); return true;
        case 0x06: nclip(); return true;
        case 0x0c: op(cmd.sf, cmd.lm); return true;
        case 0x10: dcps(cmd.sf, cmd.lm); return true;
        case 0x11: intpl(cmd.sf, cmd.lm); return true;
        case 0x12: mvmva(cmd.sf, cmd.lm, cmd.mvmvaMultiplyMatrix, cmd.mvmvaMultiplyVector, cmd.mvmvaTranslationVector); return true;
        case 0x13: ncds(cmd.sf, cmd.lm); return true;
        case 0x16: ncdt(cmd.sf, cmd.lm); return true;
        case 0x1b: nccs(cmd.sf, cmd.lm); return true;
        case 0x2a: dcpt(cmd.sf, cmd.lm); return true;
        case 0x28: sqr(); return true;
        case 0x29: dcpl(cmd.sf, cmd.lm); return true;
        case 0x2d: avsz3(); return true;
        case 0x2e: avsz4(); return true;
        case 0x30: rtpt(); return true;
        case 0x3d: gpf(cmd.lm); return true;
        case 0x3e: gpl(cmd.sf, cmd.lm); return true;
        case 0x3f: ncct(cmd.sf, cmd.lm); return true;
        default: return false;
    }
}