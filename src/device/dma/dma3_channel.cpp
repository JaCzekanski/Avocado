#include "dma3_channel.h"
#include "utils/file.h"

namespace device::dma::dmaChannel {
uint32_t DMA3Channel::readDevice() {
    uint32_t data = 0;
    if (f == nullptr) return data;
    for (int i = bytesReaded; i < bytesReaded + 4; i++) {
        data |= buffer[i] << (i * 8);
    }
    bytesReaded += 4;
    return data;
}

void DMA3Channel::beforeRead() {
    if (f == nullptr) return;
    if (bytesReaded >= (!sectorSize ? 0x800 : 0x924)) {  // 0x800 instead of 0x924 helps some games, hmm ...
        sector++;
        doSeek = true;
    }

    if (doSeek) {
        fseek(f, sector * SECTOR_SIZE, SEEK_SET);
        fread(buffer, SECTOR_SIZE, 1, f);
        doSeek = false;

        bytesReaded = 12;
        if (!sectorSize) {
            bytesReaded += 4;  // header
            bytesReaded += 4;  // subheader
            bytesReaded += 4;  // subheader copy
        }
    }

    if (verbose) printf("Sector 0x%x  ", sector);
}

DMA3Channel::DMA3Channel(int channel, System* sys) : DMAChannel(channel, sys) { verbose = false; }

bool DMA3Channel::load(const std::string& iso) {
    if (f != nullptr) {
        fclose(f);
        f = nullptr;
    }

    f = fopen(iso.c_str(), "rb");
    if (!f) {
        return false;
    }

    fseek(f, 0, SEEK_END);
    fileSize = ftell(f);
    rewind(f);
    return true;
}

void DMA3Channel::seekTo(uint32_t destSector) {
    sector = destSector - 150;
    doSeek = true;
    bytesReaded = 0;
}

size_t DMA3Channel::getIsoSize() const { return fileSize; }

void DMA3Channel::advanceSector() {
    if (bytesReaded == 0) return;
    sector++;
    doSeek = true;
    bytesReaded = 0;
}

std::array<uint8_t, DMA3Channel::SECTOR_SIZE> DMA3Channel::readSector(uint32_t sector) {
    std::array<uint8_t, SECTOR_SIZE> buffer;
    fseek(f, (sector - 150) * SECTOR_SIZE, SEEK_SET);
    fread(buffer.data(), SECTOR_SIZE, 1, f);

    return buffer;
}

uint8_t DMA3Channel::readByte() {
    beforeRead();
    return buffer[bytesReaded++];
}

}  // namespace device::dma::dmaChannel