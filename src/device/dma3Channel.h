#pragma once
#include "dmaChannel.h"
#include <src/utils/file.h>

namespace device {
namespace dma {
namespace dmaChannel {
class DMA3Channel : public DMAChannel {
    static const int SECTOR_SIZE = 2352;
    uint32_t readDevice() {
        uint32_t data = 0;
        for (int i = 0; i < 4; i++) {
            data |= ((uint8_t)fgetc(f)) << (i * 8);
        }
        bytesReaded += 4;
        return data;
    }
    void writeDevice(uint32_t data) {}

    void beforeRead() override {
        if (bytesReaded >= (!sectorSize ? 0x800 : 0x924)) {
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

        printf("Reading from sector %d, byte: %d\n", sector, ftell(f));
    }

   public:
    FILE *f;
    int sector = 0;
    bool sectorSize = false;
    bool doSeek = true;

    int bytesReaded = 0;

    int fileSize = 0;

    DMA3Channel(int channel) : DMAChannel(channel) {
        f = fopen("data/iso/marilyn.iso", "rb");
        if (!f) {
            printf("cannot open .iso");
            exit(1);
        }

        fseek(f, 0, SEEK_END);
        fileSize = ftell(f);
        rewind(f);
    }
};
}
}
}
