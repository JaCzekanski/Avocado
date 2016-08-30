#pragma once
#include "dmaChannel.h"
#include <src/utils/file.h>

namespace device {
namespace dma {
namespace dmaChannel {
class DMA3Channel : public DMAChannel {
	uint32_t readDevice() { return buffer[pointer++]; }
    void writeDevice(uint32_t data) {}

	std::vector<uint8_t> buffer;
	int pointer = 0;

   public:
    DMA3Channel(int channel) : DMAChannel(channel)
    {
		buffer = getFileContents("data/exe/header.bin");
    }
};
}
}
}
