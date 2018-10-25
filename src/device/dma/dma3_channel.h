#pragma once
#include <array>
#include "dma_channel.h"
#include "utils/file.h"

namespace device::dma::dmaChannel {
class DMA3Channel : public DMAChannel {
    static const int SECTOR_SIZE = 2352;

    FILE* f = nullptr;
    int sector = 0;
    bool doSeek = true;
    int fileSize = 0;
    int bytesReaded = 0;

    uint32_t readDevice() override;
    void beforeRead() override;
    size_t getIsoSize() const;

   public:
    uint8_t buffer[SECTOR_SIZE];
    bool sectorSize = false;

    DMA3Channel(int channel, System* sys);

    bool load(const std::string& iso);
    std::array<uint8_t, SECTOR_SIZE> readSector(uint32_t sector);
    void seekTo(uint32_t destSector);
    void advanceSector();
    uint8_t readByte();
};
}  // namespace device::dma::dmaChannel