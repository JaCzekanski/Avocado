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
        return data;
    }
    void writeDevice(uint32_t data) {}

    void beforeRead() override {
        uint32_t whichByte = sector * SECTOR_SIZE + 24;
        fseek(f, whichByte, SEEK_SET);
        sector++;
    }

   public:
    FILE *f;
    int sector = 0;

    DMA3Channel(int channel) : DMAChannel(channel) {
        f = fopen("data/iso/marilyn.iso", "rb");
        if (!f) {
            printf("cannot open .iso");
            exit(1);
        }
    }
};
}
}
}
