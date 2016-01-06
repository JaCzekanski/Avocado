#pragma once
#include "dmaChannel.h"

namespace device
{
	namespace dma
	{
		namespace dmaChannel
		{
			class DMA6Channel : public DMAChannel
			{
				uint32_t readDevice() {
					return 0xffffffff;
				}

			public: 
				DMA6Channel(int channel) : DMAChannel(channel) {}
			};
		}
	}
}