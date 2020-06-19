#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include "command.h"
#include "device/device.h"
#include "math.h"
#include "utils/logic.h"

namespace gui::debug {
class GTE;
}

class GTE {
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

    friend gui::debug::GTE;

    const std::array<uint8_t, 0x101> unrTable;
    int busToken;
    bool widescreenHack;
    bool logging;
    bool sf;  // Used for setMac and setIr functions
    bool lm;  // saved as fields to prevent passing them to every function

    // GTE registers 0-63
    gte::Vector<int16_t> v[3];
    Reg32 rgbc;
    uint16_t otz = 0;
    int16_t ir[4] = {0};
    gte::Vector<int16_t, int16_t, uint16_t> s[4];
    Reg32 rgb[3];
    uint32_t res1 = 0;     // prohibited
    int32_t mac[4] = {0};  // Sum of products
    int32_t lzcs = 0;
    int32_t lzcr = 0;

    gte::Matrix rotation;
    gte::Vector<int32_t> translation;
    gte::Matrix light;
    gte::Vector<int32_t> backgroundColor;
    gte::Matrix color;
    gte::Vector<int32_t> farColor;
    int32_t of[2] = {0};
    uint16_t h = 0;
    int16_t dqa = 0;
    int32_t dqb = 0;
    int16_t zsf3 = 0;
    int16_t zsf4 = 0;
    Flag flag;

    constexpr std::array<uint8_t, 0x101> generateUnrTable();
    void reload();

    // Internal operations and helpers
    void multiplyVectors(gte::Vector<int16_t> v1, gte::Vector<int16_t> v2, gte::Vector<int16_t> tr = gte::Vector<int16_t>(0));
    void multiplyMatrixByVector(gte::Matrix m, gte::Vector<int16_t> v, gte::Vector<int32_t> tr = gte::Vector<int32_t>(0));
    int64_t multiplyMatrixByVectorRTP(gte::Matrix m, gte::Vector<int16_t> v, gte::Vector<int32_t> tr);

    int countLeadingZeroes(uint32_t n);
    size_t countLeadingZeroes16(uint16_t n);
    int32_t clip(int32_t value, int32_t max, int32_t min, uint32_t flags = 0);

    template <int bit_size>
    void checkOverflow(int64_t value, uint32_t overflowBits, uint32_t underflowFlags);

    template <int i>
    int64_t checkMacOverflowAndExtend(int64_t value);

    template <int i>
    int64_t setMac(int64_t value);

    template <int i>
    void setIr(int32_t value, bool lm = false);

    template <int i>
    void setMacAndIr(int64_t value, bool lm = false);

    void setOtz(int64_t value);
    void pushScreenXY(int32_t x, int32_t y);
    void pushScreenZ(int32_t z);
    void pushColor();
    void pushColor(uint32_t r, uint32_t g, uint32_t b);

    uint32_t recip(uint16_t divisor);
    uint32_t divideUNR(uint32_t a, uint32_t b);
    uint32_t divide(uint16_t h, uint16_t sz3);

    // Opcodes
    void nclip();
    void ncds(int n = 0);
    void ncs(int n = 0);
    void nct();
    void nccs(int n = 0);
    void cc();
    void cdp();
    void ncdt();
    void ncct();
    void dpct();
    void dpcs(bool useRGB0 = false);
    void dcpl();
    void intpl();
    void rtps(int n = 0, bool setMAC0 = true);
    void rtpt();
    void avsz3();
    void avsz4();
    void mvmva(int mx, int vx, int tx);
    void gpf();
    void gpl();
    void sqr();
    void op();

   public:
    struct GTE_ENTRY {
        enum class MODE { read, write, func } mode;

        uint32_t n;
        uint32_t data;
    };
    std::vector<GTE_ENTRY> log;

    GTE();
    ~GTE();

    uint32_t read(uint8_t n);
    void write(uint8_t n, uint32_t d);
    void command(gte::Command& cmd);

    template <class Archive>
    void serialize(Archive& ar) {
        ar(v);
        ar(rgbc);
        ar(otz);
        ar(ir);
        ar(s);
        ar(rgb);
        ar(res1);
        ar(mac);
        ar(lzcs);
        ar(lzcr);

        ar(rotation);
        ar(translation);
        ar(light);
        ar(backgroundColor);
        ar(color);
        ar(farColor);
        ar(of);
        ar(h);
        ar(dqa);
        ar(dqb);
        ar(zsf3);
        ar(zsf4);
        ar(flag.reg);
    }
};
