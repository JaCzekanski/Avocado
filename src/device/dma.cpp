#include "dma.h"
#include "../mips.h"
#include <cstdio>

namespace device
{
	namespace dma
	{
		DMA::DMA() : dma2(2), dma6(6)
		{
		}

		void DMA::step()
		{
		}

		uint8_t DMA::read(uint32_t address)
		{
			int channel = address / 0x10;
			if (channel == 2) return dma2.read(address % 0x10); // GPU
			if (channel == 6) return dma6.read(address % 0x10); // reverse clear OT
			if (channel > 6) // control
			{
				address += 0x80;
				if (address >= 0xf0 && address < 0xf4)
				{
					return dmaControl >> ((address - 0xf0) * 8);
				}
				if (address >= 0xf4 && address < 0xf8)
				{
					return dmaStatus >> ((address - 0xf4) * 8);
				}
			}
			else {
				printf("R Unimplemented DMA channel %d\n", channel);
				__debugbreak();
				for (;;);
			}

			printf("R Unimplemented DMA address 0x%08x\n", address);
			return 0;
		}

		void DMA::write(uint32_t address, uint8_t data)
		{
			int channel = address / 0x10;
			if (channel > 6) // control
			{
				address += 0x80;
				if (address >= 0xF0 && address < 0xf4) {
					static uint32_t t = 0;
					t &= ~(0xff << 8 * (address - 0xf4));
					t |= data << 8 * (address - 0xf4);

					dmaControl = t;
					return;
				} else if (address >= 0xF4) {
					static uint32_t t = 0;
					t &= ~(0xff << 8 * (address - 0xf4));
					t |= data << 8 * (address - 0xf4);

					dmaStatus &= 0xff8000;
					dmaStatus |= t & 0xff8000;

					dmaStatus &= ~(t & 0x7f000000);
					return;
				} else {
					printf("W Unimplemented DMA address 0x%08x\n", address);
					return;
				}
			}
			
			if (channel == 2) return dma2.write(address % 0x10, data); // GPU
			if (channel == 6) return dma6.write(address % 0x10, data); // reverse clear OT

			printf("W Unimplemented DMA channel %d\n", channel);
			__debugbreak();
			for (;;);
		}
	}
}