#pragma once
#include <array>
#include <vector>
#include <optional>
#include "device/device.h"

namespace mdec {
using decodedBlock = std::array<uint32_t, 16 * 16>;

class MDEC {
    enum class Commands { None, DecodeMacroblock, SetQuantTable, SetIDCT };

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
        enum class CurrentBlock : uint32_t { Y1 = 0, Y2 = 1, Y3 = 2, Y4 = 3, Cr = 4, Cb = 5 };
        struct {
            uint32_t parameterCount : 16;
            CurrentBlock currentBlock : 3;
            uint32_t : 4;
            uint32_t outputSetBit15 : 1;    // 0 - clear, 1 - set (bit_15 only)
            uint32_t outputSigned : 1;      // 0 - unsigned, 1 - signed
            ColorDepth colorDepth : 2;      // Output depth
            uint32_t dataOutRequest : 1;    // DMA1
            uint32_t dataInRequest : 1;     // DMA0
            uint32_t commandBusy : 1;       // 0 - ready, 1 - busy
            uint32_t dataInFifoFull : 1;    // 0 - not empty, 1 - full
            uint32_t dataOutFifoEmpty : 1;  // 0 - not empty, 1 - empty
        };
        uint32_t _reg;
        uint8_t _byte[4];

        Status() : _reg(0) {}

        CurrentBlock nextBlock() const {
            if (colorDepth == ColorDepth::bit_4 || colorDepth == ColorDepth::bit_8) {
                return CurrentBlock::Y4;
            }

            switch (currentBlock) {
                case CurrentBlock::Cr: return CurrentBlock::Cb;
                case CurrentBlock::Cb: return CurrentBlock::Y1;
                case CurrentBlock::Y1: return CurrentBlock::Y2;
                case CurrentBlock::Y2: return CurrentBlock::Y3;
                case CurrentBlock::Y3: return CurrentBlock::Y4;
                case CurrentBlock::Y4: return CurrentBlock::Cr;
                default: __builtin_unreachable();
            }
        }
    };
    union Control {
        struct {
            uint32_t : 29;
            uint32_t enableDataOut : 1;  // Enables DMA1 and DataOut Request status bit
            uint32_t enableDataIn : 1;   // Enables DMA0 and DataIn Request status bit
            uint32_t reset : 1;
        };
        uint32_t _reg;
        uint8_t _byte[4];

        Control() : _reg(0) {}
    };

    int verbose;
    Command command;
    Reg32 data;
    Status status;
    Control control;

    Commands cmd;
    std::array<uint8_t, 64> luminanceQuantTable;
    std::array<uint8_t, 64> colorQuantTable;
    std::array<int16_t, 64> idctTable;

    bool color = false;  // 0 - luminance only, 1 - luminance and color
    int paramCount = 0;
    int cnt = 0;

    std::vector<uint32_t> output;
    size_t outputPtr;

    std::array<int16_t, 64> crblk = {{0}};
    std::array<int16_t, 64> cbblk = {{0}};
    std::array<int16_t, 64> yblk[4] = {{0}};  // Can be reduced to 1 instance

    // Algorithm
    void handleWord(uint16_t data);
    void yuvToRgb(decodedBlock& output, int blockX, int blockY);
    bool decodeBlock(std::array<int16_t, 64>& blk, const std::vector<uint16_t>& input, const std::array<uint8_t, 64>& table);
    void idct(std::array<int16_t, 64>& src);

   public:
    MDEC();
    void reset();
    void step();
    uint32_t read(uint32_t address);
    void handleCommand(uint8_t cmd, uint32_t data);
    void write(uint32_t address, uint32_t data);
    bool dataInRequest() const { return status.dataInRequest; }
    bool dataOutRequest() { return status.dataOutRequest; }

    template <class Archive>
    void serialize(Archive& ar) {
        ar(command._reg, data, status._reg, control._reg, cmd);
        ar(luminanceQuantTable, colorQuantTable, idctTable);
        ar(color, paramCount, cnt);
        ar(output, outputPtr);
    }
};
};  // namespace mdec
