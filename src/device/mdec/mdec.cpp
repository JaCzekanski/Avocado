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

    output.resize(0x20 * 0x5a * 20);
    outputPtr = 0;
}

uint32_t MDEC::read(uint32_t address) {
    if (address < 4) {
        uint32_t data = output[outputPtr];
        if (++outputPtr >= output.size()) {
            outputPtr = 0;
        }
        return data;
    }
    if (address >= 4 && address < 8) {
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

            // for (auto& d : output) {
            //     d = rand();
            // }
            status.colorDepth = static_cast<Status::ColorDepth>((data & 0x18000000) >> 27);
            status.outputSigned = (data & (1 << 26)) != 0;
            status.outputSetBit15 = (data & (1 << 25)) != 0;
            paramCount = data & 0xffff;

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
        case 3:  // Set Scale table
            this->cmd = Commands::SetScaleTable;

            paramCount = 64 / 2;  // 64 uint16_t
            status.commandBusy = true;

            if (verbose) printf("[MDEC] Set Scale table\n");
            break;
        default: this->cmd = Commands::None; break;
    }
}

void MDEC::write(uint32_t address, uint32_t data) {
    // printf("MDEC write @ 0x%02x: 0x%02x\n", address, data);
    if (address < 4) {
        if (paramCount != 0) {
            switch (cmd) {
                case Commands::DecodeMacroblock: break;

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

                case Commands::SetScaleTable:
                    for (int i = 0; i < 2; i++) {
                        scaleTable[cnt * 2 + i] = data >> (i * 16);
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