#pragma once
#include <utils/logic.h>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "command.h"
#include "device/device.h"
#include "math.h"

struct GTE {
    union Flag {
        enum {
            IR0_SATURATED = 1 << 12,
            SY2_SATURATED = 1 << 13,
            SX2_SATURATED = 1 << 14,
            MAC0_OVERFLOW_NEGATIVE = 1 << 15,
            MAC0_OVERFLOW_POSITIVE = 1 << 16,
            DIVIDE_OVERFLOW = 1 << 17,
            SZ3_OTZ_SATURATED = 1 << 18,
            COLOR_B_SATURATED = 1 << 19,
            COLOR_G_SATURATED = 1 << 20,
            COLOR_R_SATURATED = 1 << 21,
            IR3_SATURATED = 1 << 22,
            IR2_SATURATED = 1 << 23,
            IR1_SATURATED = 1 << 24,
            MAC3_OVERFLOW_NEGATIVE = 1 << 25,
            MAC2_OVERFLOW_NEGATIVE = 1 << 26,
            MAC1_OVERFLOW_NEGATIVE = 1 << 27,
            MAC3_OVERFLOW_POSITIVE = 1 << 28,
            MAC2_OVERFLOW_POSITIVE = 1 << 29,
            MAC1_OVERFLOW_POSITIVE = 1 << 30,
            FLAG = 1 << 31
        };
        struct {
            uint32_t : 12;  // Not used (0)
            uint32_t ir0_saturated : 1;
            uint32_t sy2_saturated : 1;
            uint32_t sx2_saturated : 1;
            uint32_t mac0_overflow_negative : 1;
            uint32_t mac0_overflow_positive : 1;
            uint32_t divide_overflow : 1;
            uint32_t sz3_otz_saturated : 1;
            uint32_t color_b_saturated : 1;
            uint32_t color_g_saturated : 1;
            uint32_t color_r_saturated : 1;
            uint32_t ir3_saturated : 1;
            uint32_t ir2_saturated : 1;
            uint32_t ir1_saturated : 1;
            uint32_t mac3_overflow_negative : 1;
            uint32_t mac2_overflow_negative : 1;
            uint32_t mac1_overflow_negative : 1;
            uint32_t mac3_overflow_positive : 1;
            uint32_t mac2_overflow_positive : 1;
            uint32_t mac1_overflow_positive : 1;
            uint32_t flag : 1;  // 30..23 + 18..13 bits ORed
        };
        uint32_t reg = 0;

        void calculate() { flag = or_range<30, 23>(reg) | or_range<18, 13>(reg); }
    };
    gte::Vector<int16_t> v[3];
    Reg32 rgbc;
    uint16_t otz = 0;
    int16_t ir[4] = {0};
    gte::Vector<int16_t, int16_t, uint16_t> s[4];
    Reg32 rgb[3];
    uint32_t res1 = 0;     // prohibited
    int32_t mac[4] = {0};  // Sum of products
    uint16_t irgb = 0;
    int32_t lzcs = 0;
    int32_t lzcr = 0;

    gte::Matrix rt;
    gte::Vector<int32_t> tr;
    gte::Matrix l;
    gte::Vector<int32_t> bk;
    gte::Matrix lr;
    gte::Vector<int32_t> fc;
    int32_t of[2] = {0};
    uint16_t h = 0;
    int16_t dqa = 0;
    int32_t dqb = 0;
    int16_t zsf3 = 0;
    int16_t zsf4 = 0;
    Flag flag;

    // Temporary, used in commands
    bool sf;
    bool lm;

    uint32_t read(uint8_t n);
    void write(uint8_t n, uint32_t d);

    void nclip();
    void ncds(bool sf, bool lm, int n = 0);
    void nccs(bool sf, bool lm, int n = 0);
    void ncdt(bool sf, bool lm);
    void ncct(bool sf, bool lm);
    void dcpt(bool sf, bool lm);
    void dcps(bool sf, bool lm);
    void dcpl(bool sf, bool lm);
    void intpl(bool sf, bool lm);
    uint32_t divide(uint16_t h, uint16_t sz3);
    uint32_t divideUNR(uint32_t a, uint32_t b);
    void rtps(int n);
    void rtpt();
    void avsz3();
    void avsz4();
    void mvmva(bool sf, bool lm, int mx, int vx, int tx);
    void gpf(bool lm);
    void gpl(bool sf, bool lm);
    void sqr();
    void op(bool sf, bool lm);

    bool command(gte::Command &cmd);

    struct GTE_ENTRY {
        enum class MODE { read, write, func } mode;

        uint32_t n;
        uint32_t data;
    };

    std::vector<GTE_ENTRY> log;

   private:
    int countLeadingZeroes(uint32_t n);
    size_t countLeadingZeroes16(uint16_t n);
    int32_t clip(int32_t value, int32_t max, int32_t min, uint32_t flags = 0);
    void check43bitsOverflow(int64_t value, uint32_t overflowBits, uint32_t underflowFlags);
    int32_t A1(int64_t value, bool sf = false);
    int32_t A2(int64_t value, bool sf = false);
    int32_t A3(int64_t value, bool sf = false);
    int32_t F(int64_t value);

    int64_t setMac(int mac, int64_t value);
    void setIr(int i, int64_t value, bool lm = false);
    void setMacAndIr(int i, int64_t value, bool lm = false);
    void setOtz(int32_t value);
    void pushScreenXY(int32_t x, int32_t y);
    void pushScreenZ(int32_t z);
    void pushColor(uint32_t r, uint32_t g, uint32_t b);
};
