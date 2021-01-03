#include "mdec.h"
#include <fmt/core.h>
#include "config.h"
#include "device/gpu/psx_color.h"

namespace mdec {

MDEC::MDEC() { reset(); }

void MDEC::step() {
    if (!startDecoding) return;

    while (!input.is_empty()) {
        if (!status.dataOutFifoEmpty) break;

        uint32_t data = input.get();
        handleWord(data & 0xffff);
        handleWord((data >> 16) & 0xffff);
    }

    if (input.is_empty()) startDecoding = false;

    status.dataInFifoFull = !input.is_empty() && status.parameterCount > 0;
    status.dataInRequest = !status.dataInFifoFull && control.enableDataIn;
}

void MDEC::reset() {
    verbose = config.debug.log.mdec;
    command._reg = 0;
    status._reg = 0x80040000;

    cmd = Commands::None;

    output.clear();
    outputPtr = 0;

    input.clear();
}

int part = 0;
template <bool dma>
uint32_t MDEC::read(uint32_t address) {
    if (address < 4) {
        if (output.empty()) {
            fmt::print("[MDEC] reading empty buffer\n");
            return 0;
        }

        const auto getPixel = [&](int ptr) -> uint32_t {
// Cast uint32_t flat buffer to uint32_t[4][8][8] blocks
#define BLOCK ((uint32_t(*)[8][8])output.data())
            if (!dma) {
                const int x = (ptr & 0b0000'0111);
                const int y = (ptr & 0b0011'1000) >> 3;
                const int b = (ptr & 0b1100'0000) >> 6;

                return BLOCK[b][y][x];
            } else {
                // Swizzle for 15bit/24bit DMA reads
                const int x = (ptr & 0b0000'0111);
                const int b = (ptr & 0b1000'0000) >> 6 | (ptr & 0b0000'1000) >> 3;
                const int y = (ptr & 0b0111'0000) >> 4;

                return BLOCK[b][y][x];
            }
#undef BLOCK
        };

        uint32_t data = 0;
        if (status.colorDepth == Status::ColorDepth::bit_24) {
            // 0:  r B G R
            // 1:  G R b g
            // 2:  b g r B
            // 3:    B G R  <- cycle continues

            if (part == 0) {
                data |= (getPixel(outputPtr) & 0xffffff);
                data |= ((getPixel(outputPtr + 1) & 0xff) << 24);
            } else if (part == 1) {
                data |= ((getPixel(outputPtr + 1) & 0xffff00) >> 8);
                data |= ((getPixel(outputPtr + 2) & 0xffff) << 16);
            } else if (part == 2) {
                data |= ((getPixel(outputPtr + 2) & 0xff0000) >> 16);
                data |= ((getPixel(outputPtr + 3) & 0xffffff) << 8);
            }

            if (part == 2) {
                part = 0;
                outputPtr += 4;
            } else {
                part++;
            }
        } else if (status.colorDepth == Status::ColorDepth::bit_15) {
            uint16_t bit15 = (uint16_t)status.outputSetBit15 << 15;
            data |= (bit15 | to15bit(getPixel(outputPtr)));
            data |= (bit15 | to15bit(getPixel(outputPtr + 1))) << 16;

            outputPtr += 2;
        } else if (status.colorDepth == Status::ColorDepth::bit_8) {
            for (int i = 0; i < 4; i++) {
                // Cheating a bit, using R channel of decoded (Y, 0, 0)
                data |= (output[outputPtr + i] & 0xff) << (i * 8);
            }
            outputPtr += 4;
        } else if (status.colorDepth == Status::ColorDepth::bit_4) {
            for (int i = 0; i < 8; i++) {
                data |= ((output[outputPtr + i] & 0xf0) >> 4) << (i * 4);
            }
            outputPtr += 8;
        } else {
            __builtin_unreachable();
        }

        if (outputPtr >= 8 * 8 * 4) {
            output.erase(output.begin(), output.begin() + 8 * 8 * 4);
            outputPtr -= 8 * 8 * 4;
        }

        if (outputPtr >= output.size()) {
            output.clear();
            outputPtr = 0;

            status.dataOutFifoEmpty = true;
            status.dataInRequest = control.enableDataIn;  // Only when cmdBusy (still decoding/waiting for data)
            status.dataOutRequest = !status.dataOutFifoEmpty && control.enableDataOut;
        }
        return data;
    }
    if (address >= 4 && address < 8) {
        // TODO: Add input fifo
        status.commandBusy = status.parameterCount > 0 || !input.is_empty() || startDecoding || !status.dataOutFifoEmpty;
        return status._reg;
    }

    return 0;
}

template uint32_t MDEC::read<false>(uint32_t);
template uint32_t MDEC::read<true>(uint32_t);

void MDEC::handleCommand(uint8_t cmd, uint32_t data) {
    this->cnt = 0;
    switch (cmd) {
        case 1:  // Decode macroblock
            this->cmd = Commands::DecodeMacroblock;

            status.colorDepth = static_cast<Status::ColorDepth>((data & 0x18000000) >> 27);
            status.outputSigned = (data & (1 << 26)) != 0;
            status.outputSetBit15 = (data & (1 << 25)) != 0;
            status.parameterCount = data & 0xffff;

            if (status.colorDepth == Status::ColorDepth::bit_4 || status.colorDepth == Status::ColorDepth::bit_8) {
                status.currentBlock = Status::CurrentBlock::Y4;
            } else {
                status.currentBlock = Status::CurrentBlock::Cr;
            }

            crblk.fill(0);
            cbblk.fill(0);
            yblk.fill(0);

            input.clear();
            startDecoding = false;

            outputPtr = 0;
            part = 0;
            output.clear();

            status.commandBusy = true;

            status.dataOutFifoEmpty = 1;
            status.dataInFifoFull = input.is_full();

            status.dataOutRequest = !status.dataOutFifoEmpty && control.enableDataOut;
            status.dataInRequest = !status.dataInFifoFull && control.enableDataIn;

            if (verbose)
                fmt::print("[MDEC] Decode macroblock (depth: {}, signed: {}, setBit15: {}, size: 0x{:x})\n",
                           static_cast<int>(status.colorDepth), static_cast<int>(status.outputSigned),
                           static_cast<int>(status.outputSetBit15), static_cast<int>(status.parameterCount));

            break;
        case 2:  // Set Quant table
            this->cmd = Commands::SetQuantTable;

            color = data & 1;
            // 64 quant table when luma only, 64+64 when color
            if (!color) {
                status.parameterCount = 64 / 4;  // 64 uint8_t
            } else {
                status.parameterCount = 128 / 4;  // 128 uint8_t
            }
            status.commandBusy = true;
            status.dataInRequest = control.enableDataIn;

            if (verbose) fmt::print("[MDEC] Set Quant table (color: {})\n", color);
            break;
        case 3:
            this->cmd = Commands::SetIDCT;

            status.parameterCount = 64 / 2;  // 64 uint16_t
            status.commandBusy = true;
            status.dataInRequest = control.enableDataIn;

            if (verbose) fmt::print("[MDEC] Set IDCT table\n");
            break;
        default: this->cmd = Commands::None; break;
    }
}

void MDEC::write(uint32_t address, uint32_t data) {
    if (address < 4) {
        if (!status.commandBusy) {
            command._reg = data;
            handleCommand(command.cmd, command.data);
            return;
        }
        status.parameterCount--;

        switch (cmd) {
            case Commands::DecodeMacroblock:
                input.add(data);

                status.dataInFifoFull = input.is_full() || status.parameterCount == 0;
                status.dataInRequest = !status.dataInFifoFull && control.enableDataIn;

                if (status.dataInFifoFull) {
                    startDecoding = true;
                }
                break;

            case Commands::SetQuantTable: {
                uint8_t* table;
                size_t base;
                if (cnt < 64 / 4) {
                    base = (cnt)*4;
                    table = luminanceQuantTable.data();
                } else if (cnt < 128 / 4) {
                    base = (cnt - 64 / 4) * 4;
                    table = colorQuantTable.data();
                } else {
                    break;
                }

                for (int i = 0; i < 4; i++) {
                    table[base + i] = data >> (i * 8);
                }
                break;
            }

            case Commands::SetIDCT:
                for (int i = 0; i < 2; i++) {
                    idctTable[cnt * 2 + i] = data >> (i * 16);
                }
                break;

            case Commands::None:
            default: break;
        }
        cnt++;
        status.commandBusy = status.parameterCount > 0 || !input.is_empty() || startDecoding || !status.dataOutFifoEmpty;
        return;
    }
    if (address >= 4 && address < 8) {
        control._reg = data;

        if (control.reset) {
            control.reset = 0;
            reset();
        }
        return;
    }
}

};  // namespace mdec
