#include <tuple>
#include "mdec.h"
#include "tables.h"

namespace mdec {

std::vector<uint16_t> input;
decodedBlock macroblock;

void MDEC::handleWord(uint16_t data) {
    input.push_back(data);

    std::array<int16_t, 64>* currentBlock;
    std::array<uint8_t, 64>* quantTable = &luminanceQuantTable;

    int isLumaStage = false;
    int blockX = 0;
    int blockY = 0;

    if (status.currentBlock == Status::CurrentBlock::Cr) {
        currentBlock = &crblk;
        quantTable = &colorQuantTable;
    } else if (status.currentBlock == Status::CurrentBlock::Cb) {
        currentBlock = &cbblk;
        quantTable = &colorQuantTable;
    } else if (status.currentBlock == Status::CurrentBlock::Y1) {
        currentBlock = &yblk[0];
        blockX = 0;
        blockY = 0;
        isLumaStage = true;
    } else if (status.currentBlock == Status::CurrentBlock::Y2) {
        currentBlock = &yblk[1];
        blockX = 0;
        blockY = 8;
        isLumaStage = true;
    } else if (status.currentBlock == Status::CurrentBlock::Y3) {
        currentBlock = &yblk[2];
        blockX = 8;
        blockY = 0;
        isLumaStage = true;
    } else if (status.currentBlock == Status::CurrentBlock::Y4) {
        currentBlock = &yblk[3];
        blockX = 8;
        blockY = 8;
        isLumaStage = true;
    } else
        __builtin_unreachable();

    if (!decodeBlock(*currentBlock, input, *quantTable)) return;

    input.clear();

    if (isLumaStage) {
        yuvToRgb(macroblock, blockX, blockY);

        if (status.currentBlock == Status::CurrentBlock::Y4) {
            output.insert(output.end(), macroblock.begin(), macroblock.end());

            status.dataOutRequest = control.enableDataOut;
        }
    }

    status.currentBlock = status.nextBlock();
}
};  // namespace mdec
