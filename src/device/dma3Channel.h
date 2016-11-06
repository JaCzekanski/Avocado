#pragma once
#include "dmaChannel.h"
#include <src/utils/file.h>

namespace device {
namespace dma {
namespace dmaChannel {
class DMA3Channel : public DMAChannel {
	uint32_t readDevice()
	{
		uint32_t data = 0;
		for (int i = 0; i<4; i++)
		{
			data |= ((uint8_t)fgetc(f)) << (i * 8);
		}
		return data;
	}
    void writeDevice(uint32_t data) {}

public:

	FILE *f;
	int pointer = 0;

    DMA3Channel(int channel) : DMAChannel(channel)
    {
		f = fopen("D:/Games/!PSX/[PSX]Metal.Gear.Solid.1/[PSX]Metal.Gear.Solid.1.CD1.iso", "rb");
		if (!f)
		{
			printf("cannot open .iso");
			exit(1);
		}
    }
};
}
}
}
