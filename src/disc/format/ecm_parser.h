#pragma once
#include <cstdio>
#include <memory>
#include <vector>
#include "disc/disc.h"
#include "ecm.h"

namespace disc::format {
class EcmParser {
   private:
    unique_ptr_file f;
    std::vector<uint8_t> data;
    uint8_t frame[2352];

    uint32_t calculateEDC(size_t addr, size_t size) const;
    void adjustEDC(size_t addr, size_t size);
    void adjustSync();
    void copySubheader();
    void calculateAndAdjustECC();
    void computeECCblock(uint8_t* src, uint32_t majorCount, uint32_t minorCount, uint32_t majorMult, uint32_t minorInc, uint8_t* dst);
    void handleMode1();
    void handleMode2Form1();
    void handleMode2Form2();

   public:
    std::unique_ptr<Ecm> parse(const char* file);
};
}  // namespace disc::format
