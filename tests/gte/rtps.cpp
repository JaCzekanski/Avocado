#include <system.h>
#include <catch.hpp>
#include "utils/string.h"

struct Entry {
    int reg;
    uint32_t data;
};

TEST_CASE("GTE_RTPS, lm=0, cv=0, v=0, mx=0, sf=1", "[GTE]") {
    auto sys = std::make_unique<System>();

    Entry initialControls[]
        = {{0, 0x00000ffb},  {1, 0xffb7ff44},  {2, 0xf9ca0ebc},  {3, 0x063700ad},  {4, 0x00000eb7},  {6, 0xfffffeac},  {7, 0x00001700},
           {9, 0x00000fa0},  {10, 0x0000f060}, {11, 0x0000f060}, {13, 0x00000640}, {14, 0x00000640}, {15, 0x00000640}, {16, 0x0bb80fa0},
           {17, 0x0fa00fa0}, {18, 0x0fa00bb8}, {19, 0x0bb80fa0}, {20, 0x00000fa0}, {24, 0x01400000}, {25, 0x00f00000}, {26, 0x00000400},
           {27, 0xfffffec8}, {28, 0x01400000}, {29, 0x00000155}, {30, 0x00000100}};

    Entry initialData[] = {{0, 0x00000b50}, {1, 0xfffff4b0}, {2, 0x00e700d5}, {3, 0xfffffe21}, {4, 0x00b90119},
                           {5, 0xfffffe65}, {6, 0x2094a539}, {8, 0x00001000}, {31, 0x00000020}};

    Entry resultControls[]
        = {{0, 0x00000ffb},  {1, 0xffb7ff44},  {2, 0xf9ca0ebc},  {3, 0x063700ad},  {4, 0x00000eb7},  {6, 0xfffffeac},  {7, 0x00001700},
           {9, 0x00000fa0},  {10, 0x0000f060}, {11, 0x0000f060}, {13, 0x00000640}, {14, 0x00000640}, {15, 0x00000640}, {16, 0x0bb80fa0},
           {17, 0x0fa00fa0}, {18, 0x0fa00bb8}, {19, 0x0bb80fa0}, {20, 0x00000fa0}, {24, 0x01400000}, {25, 0x00f00000}, {26, 0x00000400},
           {27, 0xfffffec8}, {28, 0x01400000}, {29, 0x00000155}, {30, 0x00000100}, {31, 0x80004000}};

    Entry resultData[]
        = {{0, 0x00000b50},  {1, 0xfffff4b0},  {2, 0x00e700d5},  {3, 0xfffffe21},  {4, 0x00b90119},  {5, 0xfffffe65},  {6, 0x2094a539},
           {8, 0x00000e08},  {9, 0x00000bd1},  {10, 0x000002dc}, {11, 0x00000d12}, {14, 0x01d003ff}, {15, 0x01d003ff}, {19, 0x00000d12},
           {24, 0x00e08388}, {25, 0x00000bd1}, {26, 0x000002dc}, {27, 0x00000d12}, {28, 0x000068b7}, {29, 0x000068b7}, {31, 0x00000020}};

    for (int i = 0; i < sizeof(initialControls) / sizeof(Entry); i++) {
        sys->cpu->gte.write(32 + initialControls[i].reg, initialControls[i].data);
    }

    for (int i = 0; i < sizeof(initialData) / sizeof(Entry); i++) {
        sys->cpu->gte.write(initialData[i].reg, initialData[i].data);
    }

    gte::Command cmd(0x00080001);
    sys->cpu->gte.command(cmd);

    for (int i = 0; i < sizeof(resultControls) / sizeof(Entry); i++) {
        SECTION(string_format("Control reg %d", resultControls[i].reg)) {
            CHECK(sys->cpu->gte.read(32 + resultControls[i].reg) == resultControls[i].data);
        }
    }

    for (int i = 0; i < sizeof(resultData) / sizeof(Entry); i++) {
        SECTION(string_format("Data reg %d", resultData[i].reg)) { CHECK(sys->cpu->gte.read(resultData[i].reg) == resultData[i].data); }
    }
}

TEST_CASE("GTE_OP GTE_RTPS, lm=0, cv=0, v=0, mx=0, sf=1 random", "[GTE]") {
    auto sys = std::make_unique<System>();

    Entry initialControls[]
        = {{0, 0xff35cdf4},  {1, 0xf8acd6a6},  {2, 0x1954aa70},  {3, 0xae7b5062},  {4, 0x00000c63},  {5, 0xcad4cc39},  {6, 0xb9c11958},
           {7, 0xa942b312},  {8, 0xaf436779},  {9, 0x3c2d507a},  {10, 0x95f99741}, {11, 0x72413224}, {12, 0x0000499d}, {13, 0x0a37d280},
           {14, 0xdbe8feec}, {15, 0x2395909a}, {16, 0x47364c98}, {17, 0x795c2ed7}, {18, 0x637e48f4}, {19, 0x89557da5}, {20, 0xffff997a},
           {21, 0x690eb551}, {22, 0x3dfb368e}, {23, 0x2bbe355f}, {24, 0x307c9d22}, {25, 0x030c876b}, {26, 0x00003b7d}, {27, 0x0000765a},
           {28, 0x228c2901}, {29, 0xffffe86f}, {30, 0xffffaf93}, {31, 0xc741f000}};

    Entry initialData[] = {{0, 0x91d5c574},  {1, 0xffffdf9c},  {2, 0xcea213bc},  {3, 0x0000143e},  {4, 0x2360a947},  {5, 0x00003248},
                           {6, 0x1747e72e},  {7, 0x0000cc08},  {8, 0x0000381d},  {9, 0xffffe2ff},  {10, 0xffffe0f8}, {11, 0xffffe1b6},
                           {12, 0x9da7438d}, {13, 0xff60f0ed}, {14, 0xbf5961ab}, {15, 0xbf5961ab}, {16, 0x0000b1c1}, {17, 0x0000dda6},
                           {18, 0x0000ce75}, {19, 0x0000b2d1}, {20, 0xdb01b77a}, {21, 0x19cd28cd}, {22, 0x1a75d97a}, {23, 0xe91dc0ad},
                           {24, 0x764e464f}, {25, 0x4aa5a1e5}, {26, 0x3b1a1977}, {27, 0x39fb3f5f}, {30, 0xfe8de0c9}, {31, 0x00000007}};

    Entry resultControls[]
        = {{0, 0xff35cdf4},  {1, 0xf8acd6a6},  {2, 0x1954aa70},  {3, 0xae7b5062},  {4, 0x00000c63},  {5, 0xcad4cc39},  {6, 0xb9c11958},
           {7, 0xa942b312},  {8, 0xaf436779},  {9, 0x3c2d507a},  {10, 0x95f99741}, {11, 0x72413224}, {12, 0x0000499d}, {13, 0x0a37d280},
           {14, 0xdbe8feec}, {15, 0x2395909a}, {16, 0x47364c98}, {17, 0x795c2ed7}, {18, 0x637e48f4}, {19, 0x89557da5}, {20, 0xffff997a},
           {21, 0x690eb551}, {22, 0x3dfb368e}, {23, 0x2bbe355f}, {24, 0x307c9d22}, {25, 0x030c876b}, {26, 0x00003b7d}, {27, 0x0000765a},
           {28, 0x228c2901}, {29, 0xffffe86f}, {30, 0xffffaf93}, {31, 0x81c7f000}};
    Entry resultData[] = {{0, 0x91d5c574},  {1, 0xffffdf9c},  {2, 0xcea213bc},  {3, 0x0000143e},  {4, 0x2360a947},  {5, 0x00003248},
                          {6, 0x1747e72e},  {7, 0x0000cc08},  {8, 0x00001000},  {9, 0xffff8000},  {10, 0xffff8000}, {11, 0xffff8000},
                          {12, 0xff60f0ed}, {13, 0xbf5961ab}, {14, 0xfc00fc00}, {15, 0xfc00fc00}, {16, 0x0000dda6}, {17, 0x0000ce75},
                          {18, 0x0000b2d1}, {20, 0xdb01b77a}, {21, 0x19cd28cd}, {22, 0x1a75d97a}, {23, 0xe91dc0ad}, {24, 0x0f3fb2a7},
                          {25, 0xcad5dc86}, {26, 0xb9c34e06}, {27, 0xa943a529}, {30, 0xfe8de0c9}, {31, 0x00000007}};

    for (int i = 0; i < sizeof(initialControls) / sizeof(Entry); i++) {
        sys->cpu->gte.write(32 + initialControls[i].reg, initialControls[i].data);
    }

    for (int i = 0; i < sizeof(initialData) / sizeof(Entry); i++) {
        sys->cpu->gte.write(initialData[i].reg, initialData[i].data);
    }

    gte::Command cmd(0x00080001);
    sys->cpu->gte.command(cmd);

    for (int i = 0; i < sizeof(resultControls) / sizeof(Entry); i++) {
        SECTION(string_format("Control reg %d", resultControls[i].reg)) {
            CHECK(sys->cpu->gte.read(32 + resultControls[i].reg) == resultControls[i].data);
        }
    }

    for (int i = 0; i < sizeof(resultData) / sizeof(Entry); i++) {
        SECTION(string_format("Data reg %d", resultData[i].reg)) { CHECK(sys->cpu->gte.read(resultData[i].reg) == resultData[i].data); }
    }
}
