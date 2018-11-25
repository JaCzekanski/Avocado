#pragma once
#include <array>
#include <vector>
#include "device/device.h"

namespace mdec {
class MDEC {
    enum class Commands { None, DecodeMacroblock, SetQuantTable, SetScaleTable };

    union Command {
        struct {
            uint32_t data : 29;
            uint32_t cmd : 3;
        };
        uint32_t _reg;
        uint8_t _byte[4];

        Command() : _reg(0) {}
    };
    union Status {
        enum class ColorDepth : uint32_t { bit_4, bit_8, bit_24, bit_15 };
        enum class CurrentBlock : uint32_t { Y1, Y2, Y3, Y4, Cr, Cb };
        struct {
            uint32_t parameterCount : 15;
            CurrentBlock currentBlock : 3;
            uint32_t : 4;
            uint32_t outputSetBit15 : 1;    // 0 - clear, 1 - set (bit_15 only)
            uint32_t outputSigned : 1;      // 0 - unsigned, 1 - signed
            ColorDepth colorDepth : 2;      // Output depth
            uint32_t dataOutRequest : 1;    // DMA1
            uint32_t dataInRequest : 1;     // DMA0
            uint32_t commandBusy : 1;       // 0 - ready, 1 - busy
            uint32_t dataInFifoEmpty : 1;   // 0 - not empty, 1 - full
            uint32_t dataOutFifoEmpty : 1;  // 0 - not empty, 1 - empty
        };
        uint32_t _reg;
        uint8_t _byte[4];

        Status() : _reg(0) {}
    };

    static const uint32_t BASE_ADDRESS = 0x1f801820;

    int verbose;
    Command command;
    Reg32 data;
    Status status;
    Reg32 _control;

    Commands cmd;
    std::array<uint8_t, 64> luminanceQuantTable;
    std::array<uint8_t, 64> colorQuantTable;
    std::array<int16_t, 64> scaleTable;

    bool color = false;  // 0 - luminance only, 1 - luminance and color
    int paramCount = 0;
    int cnt = 0;

    void reset();

    std::vector<uint32_t> output;
    size_t outputPtr;

   public:
    MDEC();
    void step();
    uint32_t read(uint32_t address);
    void handleCommand(uint8_t cmd, uint32_t data);
    void write(uint32_t address, uint32_t data);
};
};  // namespace mdec