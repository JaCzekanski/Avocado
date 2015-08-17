#pragma once
#include "device.h"
#include "gpu.h"

namespace device
{
	namespace dma
	{
		class DMA : public Device
		{
			uint32_t dmaControl;
			uint32_t dmaStatus;
			uint32_t dma2Control;
			uint32_t dma6Control;

			void *_cpu = nullptr;
			gpu::GPU *gpu = nullptr;

		public:
			void step();
			uint8_t read(uint32_t address);
			void write(uint32_t address, uint8_t data);


			void setCPU(void *cpu)
			{
				this->_cpu = cpu;
			}

			void setGPU(gpu::GPU *gpu) 
			{
				this->gpu = gpu;
			}
		};
	}
}