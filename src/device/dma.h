#pragma once
#include "device.h"
#include "gpu.h"

namespace device
{
	namespace dma
	{

		// DMA base address
		union MADDR
		{
			struct
			{
				uint32_t address : 24;
				uint32_t zero : 8;
			};
			uint32_t _reg;
			uint8_t _byte[4];

			MADDR() : _reg(0) {}
		};

		// DMA Block Control
		union BCR
		{
			union
			{
				struct
				{
					uint32_t wordCount : 16;
					uint32_t zero : 16;
				} syncMode0;
				struct
				{
					uint32_t blockSize : 16;
					uint32_t blockCount : 16;
				} syncMode1;
			};
			uint32_t _reg;
			uint8_t _byte[4];

			BCR() : _reg(0) {}
		};

		// DMA Channel Control
		union CHCR
		{
			enum class TransferDirection : uint8_t { toMainRam = 0, fromMainRam = 1 };
			enum class MemoryAddressStep : uint8_t { forward = 0, backward = 1 };
			enum class ChoppingEnable : uint8_t { normal = 0, chopping = 1 };
			enum class Enabled : uint8_t { stop = 0, start = 1 };
			enum class StartTrigger : uint8_t { clear = 0, automatic = 0, manual = 1 };

			struct
			{
				TransferDirection transferDirection : 1;
				MemoryAddressStep memoryAddressStep : 1;
				uint8_t unused1 : 6;
				ChoppingEnable choppingEnable : 1;
				uint8_t unused2 : 5;
				uint8_t choppingDmaWindowSize : 3; // Chopping DMA Window Size (1 SHL N words)
				uint8_t unused3 : 1;
				uint8_t choppingCpuWindowSize : 3; // Chopping CPU Window Size(1 SHL N clks)
				uint8_t unused4 : 1;
				Enabled enabled : 1; // stopped/completed, start/enable/busy
				uint8_t unused5 : 3;
				StartTrigger startTrigger : 1;
				uint8_t unused6 : 3;
			};
			uint32_t _reg;
			uint8_t _byte[4];

			CHCR() : _reg(0) {}
		};



		class DMA : public Device
		{
			uint32_t dmaControl;
			uint32_t dmaStatus;
			uint32_t dma2Control;
			MADDR dma2Address;
			uint32_t dma2Count = 0;
			CHCR dma6Control;
			MADDR dma6Address;
			BCR dma6Count;

			void *_cpu = nullptr;
			gpu::GPU *gpu = nullptr;

			void writeDma2(uint32_t address, uint8_t data);
			void writeDma6(uint32_t address, uint8_t data);
		public:
			DMA();
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