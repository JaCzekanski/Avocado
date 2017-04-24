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
        for (int i = 0; i < 4; i++) {
            data |= ((uint8_t)fgetc(f)) << (i * 8);
        }
        bytesReaded += 4;
        return data;
    }
    void writeDevice(uint32_t data) override {}

    void beforeRead() override {
        if (bytesReaded >= (!sectorSize ? 0x800 : 0x924)) {  // 0x800 instead of 0x924 helps some games, hmm ...
            bytesReaded = 0;
            sector++;
            doSeek = true;
        }

        if (doSeek) {
            uint32_t whichByte = sector * SECTOR_SIZE;
            whichByte += 12;  // sync bits
            if (!sectorSize) {
                whichByte += 4;  // header
                whichByte += 4;  // subheader
                whichByte += 4;  // subheader copy
            }

            fseek(f, whichByte, SEEK_SET);
            doSeek = false;
        }

        printf("Sector 0x%x  ", sector);
    }

    FILE *f;
    int sector = 0;
    bool doSeek = true;
    int bytesReaded = 0;
    int fileSize = 0;

   public:
    bool sectorSize = false;

    DMA3Channel(int channel) : DMAChannel(channel) {
        extern const char *CD_FILE;
        f = fopen(CD_FILE, "rb");
        if (!f) {
            printf("cannot open .iso");
            exit(1);
        }

        fseek(f, 0, SEEK_END);
        fileSize = ftell(f);
        rewind(f);
    }

    void seekTo(uint32_t destSector) {
        sector = destSector;
        doSeek = true;
        bytesReaded = 0;
    }

    size_t getIsoSize() const { return fileSize; }
};
}
}
}
