#pragma once
#include "dmaChannel.h"
#include <src/utils/file.h>

namespace device {
namespace dma {
namespace dmaChannel {
class DMA3Channel : public DMAChannel {
    static const int SECTOR_SIZE = 2352;
    uint32_t readDevice() override {
        uint32_t data = 0;
        if (f == nullptr) return data;
        for (int i = bytesReaded; i < bytesReaded + 4; i++) {
            data |= buffer[i] << (i * 8);
        }
        bytesReaded += 4;
        return data;
    }
    void writeDevice(uint32_t data) override {}

    void beforeRead() override {
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

    FILE* f = nullptr;
    int sector = 0;
    bool doSeek = true;
    int fileSize = 0;

   public:
    int bytesReaded = 0;
    uint8_t buffer[SECTOR_SIZE];
    bool sectorSize = false;

    DMA3Channel(int channel) : DMAChannel(channel) { verbose = true; }

    bool load(const std::string& iso) {
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

    void seekTo(uint32_t destSector) {
        sector = destSector;
        doSeek = true;
        bytesReaded = 0;
    }

    size_t getIsoSize() const { return fileSize; }
    void advanceSector() {
        if (bytesReaded == 0) return;
        sector++;
        doSeek = true;
        bytesReaded = 0;
    }

    uint8_t readByte() {
        beforeRead();
        return buffer[bytesReaded++];
    }
};
}
}
}
