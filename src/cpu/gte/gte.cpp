#include "gte.h"
#include <algorithm>
#include <cmath>

template <size_t from, size_t to>
bool orRange(uint32_t v) {
    static_assert(from >= 0 && from < 32, "from out of range");
    static_assert(to >= 0 && to < 32, "to out of range");

    const size_t max = std::max(from, to);
    const size_t min = std::min(from, to);

    bool result = false;
    for (size_t i = min; i <= max; i++) {
        result |= (v & (1 << i)) != 0;
    }
    return result;
}

uint32_t GTE::read(uint8_t n) {
    uint32_t ret = 0;
    switch (n) {
        // Data
        case 0:
            ret = ((uint16_t)v[0].y << 16) | (uint16_t)v[0].x;
            break;
        case 1:
            ret = (int32_t)v[0].z;
            break;
        case 2:
            ret = ((uint16_t)v[1].y << 16) | (uint16_t)v[1].x;
            break;
        case 3:
            ret = (int32_t)v[1].z;
            break;
        case 4:
            ret = ((uint16_t)v[2].y << 16) | (uint16_t)v[2].x;
            break;
        case 5:
            ret = (int32_t)v[2].z;
            break;
        case 6:
            ret = rgbc._reg;
            break;
        case 7:
            ret = otz;
            break;

        case 8:
            ret = (int32_t)ir[0];
            break;
        case 9:
            ret = (int32_t)ir[1];
            break;
        case 10:
            ret = (int32_t)ir[2];
            break;
        case 11:
            ret = (int32_t)ir[3];
            break;
        case 12:
            ret = ((uint16_t)s[0].y << 16) | (uint16_t)s[0].x;
            break;
        case 13:
            ret = ((uint16_t)s[1].y << 16) | (uint16_t)s[1].x;
            break;
        case 14:
        case 15:
            ret = ((uint16_t)s[2].y << 16) | (uint16_t)s[2].x;
            break;

        case 16:
            ret = (uint16_t)s[0].z;
            break;
        case 17:
            ret = (uint16_t)s[1].z;
            break;
        case 18:
            ret = (uint16_t)s[2].z;
            break;
        case 19:
            ret = (uint16_t)s[3].z;
            break;
        case 20:
            ret = rgb[0]._reg;
            break;
        case 21:
            ret = rgb[1]._reg;
            break;
        case 22:
            ret = rgb[2]._reg;
            break;
        case 23:
            ret = res1;
            break;

        case 24:
            ret = mac[0];
            break;
        case 25:
            ret = mac[1];
            break;
        case 26:
            ret = mac[2];
            break;
        case 27:
            ret = mac[3];
            break;
        case 28:
        case 29:
            irgb = clip(ir[1] / 0x80, 0x1f, 0x00);
            irgb |= clip(ir[2] / 0x80, 0x1f, 0x00) << 5;
            irgb |= clip(ir[3] / 0x80, 0x1f, 0x00) << 10;
            ret = irgb;
            break;
        case 30:
            ret = lzcs;
            break;
        case 31:
            ret = lzcr;
            break;

        case 32:
            ret = ((uint16_t)rt.v12 << 16) | (uint16_t)rt.v11;
            break;
        case 33:
            ret = ((uint16_t)rt.v21 << 16) | (uint16_t)rt.v13;
            break;
        case 34:
            ret = ((uint16_t)rt.v23 << 16) | (uint16_t)rt.v22;
            break;
        case 35:
            ret = ((uint16_t)rt.v32 << 16) | (uint16_t)rt.v31;
            break;
        case 36:
            ret = (int32_t)rt.v33;
            break;
        case 37:
            ret = tr.x;
            break;
        case 38:
            ret = tr.y;
            break;
        case 39:
            ret = tr.z;
            break;

        case 40:
            ret = ((uint16_t)l.v12 << 16) | (uint16_t)l.v11;
            break;
        case 41:
            ret = ((uint16_t)l.v21 << 16) | (uint16_t)l.v13;
            break;
        case 42:
            ret = ((uint16_t)l.v23 << 16) | (uint16_t)l.v22;
            break;
        case 43:
            ret = ((uint16_t)l.v32 << 16) | (uint16_t)l.v31;
            break;
        case 44:
            ret = (int32_t)l.v33;
            break;
        case 45:
            ret = bk.r;
            break;
        case 46:
            ret = bk.g;
            break;
        case 47:
            ret = bk.b;
            break;

        case 48:
            ret = ((uint16_t)lr.v12 << 16) | (uint16_t)lr.v11;
            break;
        case 49:
            ret = ((uint16_t)lr.v21 << 16) | (uint16_t)lr.v13;
            break;
        case 50:
            ret = ((uint16_t)lr.v23 << 16) | (uint16_t)lr.v22;
            break;
        case 51:
            ret = ((uint16_t)lr.v32 << 16) | (uint16_t)lr.v31;
            break;
        case 52:
            ret = (int32_t)lr.v33;
            break;
        case 53:
            ret = fc.r;
            break;
        case 54:
            ret = fc.g;
            break;
        case 55:
            ret = fc.b;
            break;

        case 56:
            ret = of[0];
            break;
        case 57:
            ret = of[1];
            break;
        case 58:
            // gte_bug: H is sign extended on read
            ret = (int32_t)(int16_t)h;
            break;
        case 59:
            ret = (int32_t)dqa;
            break;
        case 60:
            ret = dqb;
            break;
        case 61:
            // gte_bug?: sign extended
            ret = (int32_t)(int16_t)zsf3;
            break;
        case 62:
            // gte_bug?: sign extended
            ret = (int32_t)(int16_t)zsf4;
            break;
        case 63: {
            bool errorFlag = orRange<30, 23>(flag) | orRange<18, 13>(flag);
            flag &= ~(1 << 31);

            ret = (errorFlag << 31) | flag;
            break;
        }
        default:
            ret = 0;
            break;
    }

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
        case 1:
            v[0].z = d;
            break;
        case 2:
            v[1].y = d >> 16;
            v[1].x = d;
            break;
        case 3:
            v[1].z = d;
            break;
        case 4:
            v[2].y = d >> 16;
            v[2].x = d;
            break;
        case 5:
            v[2].z = d;
            break;
        case 6:
            rgbc._reg = d;
            break;
        case 7:
            otz = d;
            break;

        case 8:
            ir[0] = d;
            break;
        case 9:
            ir[1] = d;
            break;
        case 10:
            ir[2] = d;
            break;
        case 11:
            ir[3] = d;
            break;
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

        case 16:
            s[0].z = d;
            break;
        case 17:
            s[1].z = d;
            break;
        case 18:
            s[2].z = d;
            break;
        case 19:
            s[3].z = d;
            break;
        case 20:
            rgb[0]._reg = d;
            break;
        case 21:
            rgb[1]._reg = d;
            break;
        case 22:
            rgb[2]._reg = d;
            break;
        case 23:
            res1 = d;
            break;

        case 24:
            mac[0] = d;
            break;
        case 25:
            mac[1] = d;
            break;
        case 26:
            mac[2] = d;
            break;
        case 27:
            mac[3] = d;
            break;
        case 28:
            irgb = d & 0x00007fff;
            ir[1] = (d & 0x1f) * 0x80;
            ir[2] = ((d >> 5) & 0x1f) * 0x80;
            ir[3] = ((d >> 10) & 0x1f) * 0x80;
            break;
        case 29:
            break;
        case 30:
            lzcs = d;
            lzcr = countLeadingZeroes(lzcs);
            break;
        case 31:
            break;

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
        case 36:
            rt.v33 = d;
            break;
        case 37:
            tr.x = d;
            break;
        case 38:
            tr.y = d;
            break;
        case 39:
            tr.z = d;
            break;

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
        case 44:
            l.v33 = d;
            break;
        case 45:
            bk.r = d;
            break;
        case 46:
            bk.g = d;
            break;
        case 47:
            bk.b = d;
            break;

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
        case 52:
            lr.v33 = d;
            break;
        case 53:
            fc.r = d;
            break;
        case 54:
            fc.g = d;
            break;
        case 55:
            fc.b = d;
            break;

        case 56:
            of[0] = d;
            break;
        case 57:
            of[1] = d;
            break;
        case 58:
            h = d;
            break;
        case 59:
            dqa = d;
            break;
        case 60:
            dqb = d;
            break;
        case 61:
            zsf3 = d;
            break;
        case 62:
            zsf4 = d;
            break;
        case 63:
            flag = d & 0x7FFFF000;
            break;
        default:
            return;
    }
}

bool GTE::command(gte::Command& cmd) {
    flag = 0;
    this->sf = cmd.sf;
    this->lm = cmd.lm;

    switch (cmd.cmd) {
        case 0x01:
            rtps(0);
            return true;
        case 0x06:  // TODO: Check (MoH)
            nclip();
            return true;
        case 0x0c:
            op(cmd.sf, cmd.lm);
            return true;

        case 0x10:
            dcps(cmd.sf, cmd.lm);
            return true;

        case 0x11:
            intpl(cmd.sf, cmd.lm);
            return true;

        case 0x12:  // TODO: Check (MoH)
            mvmva(cmd.sf, cmd.lm, cmd.mvmvaMultiplyMatrix, cmd.mvmvaMultiplyVector, cmd.mvmvaTranslationVector);
            return true;
        case 0x13:
            ncds(cmd.sf, cmd.lm);
            return true;

        case 0x16:
            ncdt(cmd.sf, cmd.lm);
            return true;

        case 0x1b:
            nccs(cmd.sf, cmd.lm);
            return true;

        case 0x2a:  // TODO: Check (MoH)
            dcpt(cmd.sf, cmd.lm);
            return true;

        case 0x28:
            sqr();
            return true;

        case 0x29:
            dcpl(cmd.sf, cmd.lm);
            return true;
        case 0x2d:  // TODO: Check (MoH)
            avsz3();
            return true;
        case 0x2e:
            avsz4();
            return true;
        case 0x30:
            rtpt();
            return true;
        case 0x3d:
            gpf(cmd.lm);
            return true;
        case 0x3e:
            gpl(cmd.sf, cmd.lm);
            return true;
        case 0x3f:
            ncct(cmd.sf, cmd.lm);
            return true;
        default:
            return false;
    }
}