#include "mdec.h"
#include <cassert>
#include <cstdio>
#include "config.h"

namespace mdec {

MDEC::MDEC() { reset(); }

void MDEC::step() {}

void MDEC::reset() {
    verbose = config["debug"]["log"]["mdec"];
    command._reg = 0;
    status._reg = 0x80040000;

    cmd = Commands::None;

    outputPtr = 0;
}

int part = 0;
uint32_t MDEC::read(uint32_t address) {
    if (address < 4) {
        // 0:  r B G R
        // 1:  G R b g
        // 2:  b g r B
        // 3:    B G R  <- cycle continues

        uint32_t data;
        if (part == 0)
            data = (output[outputPtr] & 0xffffff) | ((output[outputPtr + 1] & 0xff) << 24);
        else if (part == 1)
            data = ((output[outputPtr + 1] & 0xffff00) >> 8) | ((output[outputPtr + 2] & 0xffff) << 16);
        else if (part == 2)
            data = ((output[outputPtr + 2] & 0xff0000) >> 16) | ((output[outputPtr + 3] & 0xffffff) << 8);

        if (part == 2) {
            part = 0;
            outputPtr += 4;
        } else {
            part++;
        }

        if (outputPtr >= output.size()) {
            outputPtr = 0;
        }
        return data;
    }
    if (address >= 4 && address < 8) {
        // TODO: Handle fifo bits
        status.dataOutFifoEmpty = outputPtr >= output.size();
        status.dataInFifoFull = 0;
        status.commandBusy = 0;
        return status._reg;
    }

    assert(false && "UNHANDLED MDEC READ");
    return 0;
}

void MDEC::handleCommand(uint8_t cmd, uint32_t data) {
    this->cnt = 0;
    switch (cmd) {
        case 1:  // Decode macroblock
            this->cmd = Commands::DecodeMacroblock;

            status.colorDepth = static_cast<Status::ColorDepth>((data & 0x18000000) >> 27);
            status.outputSigned = (data & (1 << 26)) != 0;
            status.outputSetBit15 = (data & (1 << 25)) != 0;
            paramCount = data & 0xffff;

            input.resize(paramCount * 2);  // 16bit, so paramCount * 2
            outputPtr = 0;
            part = 0;
            output.resize(0);

            status.commandBusy = true;

            if (verbose)
                printf("[MDEC] Decode macroblock (depth: %d, signed: %d, setBit15: %d, size: 0x%0x)\n", status.colorDepth,
                       status.outputSigned, status.outputSetBit15, paramCount);

            break;
        case 2:  // Set Quant table
            this->cmd = Commands::SetQuantTable;

            color = data & 1;
            // 64 quant table when luma only, 64+64 when color
            if (!color) {
                paramCount = 64 / 4;  // 64 uint8_t
            } else {
                paramCount = 128 / 4;  // 128 uint8_t
            }
            status.commandBusy = true;

            if (verbose) printf("[MDEC] Set Quant table (color: %d)\n", color);
            break;
        case 3:
            this->cmd = Commands::SetIDCT;

            paramCount = 64 / 2;  // 64 uint16_t
            status.commandBusy = true;

            if (verbose) printf("[MDEC] Set IDCT table\n");
            break;
        default: this->cmd = Commands::None; break;
    }
}

void MDEC::write(uint32_t address, uint32_t data) {
    // printf("MDEC write @ 0x%02x: 0x%02x\n", address, data);
    if (address < 4) {
        if (paramCount != 0) {
            switch (cmd) {
                case Commands::DecodeMacroblock:
                    // Macroblock consist of
                    input[cnt * 2] = data & 0xffff;
                    input[cnt * 2 + 1] = (data >> 16) & 0xffff;
                    if (paramCount == 1) {
                        // TODO: Pass macroblock, not raw pointer
                        decodeMacroblocks();
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
            paramCount--;
            status.parameterCount = (paramCount - 1) & 0xffff;
        } else {
            command._reg = data;
            handleCommand(command.cmd, command.data);
            status.parameterCount = (paramCount - 1) & 0xffff;
        }
        return;
    }
    if (address >= 4 && address < 8) {
        _control._reg = data;

        if (_control.getBit(31)) reset();
        status.dataInRequest = _control.getBit(28);
        status.dataOutRequest = _control.getBit(27);
        return;
    }

    assert(false && "UNHANDLED MDEC WRITE");
}

};  // namespace mdec
