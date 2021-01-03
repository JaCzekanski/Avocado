#include <tuple>
#include <fmt/core.h>
#include <magic_enum.hpp>
#include "mdec.h"
#include "tables.h"

namespace mdec {

std::vector<uint16_t> inputBuffer;
void MDEC::handleWord(uint16_t data) {
    inputBuffer.push_back(data);

    std::array<int16_t, 64>* currentBlock;
    std::array<uint8_t, 64>* quantTable = &luminanceQuantTable;

    int isLumaStage = false;

    if (status.currentBlock == Status::CurrentBlock::Cr) {
        currentBlock = &crblk;
        quantTable = &colorQuantTable;
    } else if (status.currentBlock == Status::CurrentBlock::Cb) {
        currentBlock = &cbblk;
        quantTable = &colorQuantTable;
    } else if (status.currentBlock == Status::CurrentBlock::Y1) {
        currentBlock = &yblk;
        isLumaStage = true;
    } else if (status.currentBlock == Status::CurrentBlock::Y2) {
        currentBlock = &yblk;
        isLumaStage = true;
    } else if (status.currentBlock == Status::CurrentBlock::Y3) {
        currentBlock = &yblk;
        isLumaStage = true;
    } else if (status.currentBlock == Status::CurrentBlock::Y4) {
        currentBlock = &yblk;
        isLumaStage = true;
    } else
        __builtin_unreachable();

    // TODO: Convert decodeBlock to rle with state machine
    // and remove input vector
    if (!decodeBlock(*currentBlock, inputBuffer, *quantTable)) return;

    inputBuffer.clear();

    if (isLumaStage) {
        DecodedBlock decodedBlock = yuvToRgb(status.currentBlock);
        output.insert(output.end(), decodedBlock.begin(), decodedBlock.end());

        if (status.currentBlock == Status::CurrentBlock::Y4) {
            status.dataOutFifoEmpty = false;
            status.dataOutRequest = !status.dataOutFifoEmpty && control.enableDataOut;

            status.dataInRequest = 0;
        }
    }

    status.currentBlock = status.nextBlock();
}
};  // namespace mdec
