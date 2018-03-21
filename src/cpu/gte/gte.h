#pragma once
#include <cstdint>
#include <vector>
#include "command.h"
#include "device/device.h"
#include "math.h"

struct GTE {
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
    uint32_t flag = 0;

    // Temporary, used in commands
    bool sf;
    bool lm;

    uint32_t read_(uint8_t n);
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
    int32_t divide(uint16_t h, uint16_t sz3);
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
    int32_t clip(int32_t value, int32_t max, int32_t min, uint32_t flags = 0);
    void check43bitsOverflow(int64_t value, uint32_t overflowBits, uint32_t underflowFlags);
    int32_t A1(int64_t value, bool sf = false);
    int32_t A2(int64_t value, bool sf = false);
    int32_t A3(int64_t value, bool sf = false);
    int32_t F(int64_t value);

    int64_t setMac(int mac, int64_t value);
    void setMacAndIr(int i, int64_t value, bool lm = false);
    void setOtz(int32_t value);
    void pushScreenXY(int32_t x, int32_t y);
    void pushScreenZ(uint32_t z);
    void pushColor(uint32_t r, uint32_t g, uint32_t b);
};