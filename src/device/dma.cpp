#include "dma.h"
#include "../mips.h"
#include <cstdio>

namespace device
{
	namespace dma
	{
		DMA::DMA()
		{
		}

		void DMA::step()
		{
		}

		uint8_t DMA::read(uint32_t address)
		{
			address += 0x80;
			if (address >= 0xa8 && address < 0xac) 
			{
				return dma2Control >> ((address - 0xa8) * 8);
			}
			if (address >= 0xe8 && address < 0xec) 
			{
				return dma6Control._reg >> ((address - 0xe8) * 8);
			}
			if (address >= 0xf0 && address < 0xf4)
			{
				return dmaControl >> ((address - 0xf0) * 8);
			}
			if (address >= 0xf4 && address < 0xf8)
			{
				return dmaStatus >> ((address - 0xf4) * 8);
			}

			printf("R Unimplemented DMA address 0x%08x\n", address);
			return 0;
		}

		void DMA::write(uint32_t address, uint8_t data)
		{
			mips::CPU *cpu = (mips::CPU*)_cpu;

			int channel = address / 0x10;
			if (channel > 6) // control
			{
				address += 0x80;
				if (address >= 0xF0 && address < 0xf4) {
					static uint32_t t = 0;
					t &= ~(0xff << 8 * (address - 0xf4));
					t |= data << 8 * (address - 0xf4);

					dmaControl = t;
				} else if (address >= 0xF4) {
					static uint32_t t = 0;
					t &= ~(0xff << 8 * (address - 0xf4));
					t |= data << 8 * (address - 0xf4);

					dmaStatus &= 0xff8000;
					dmaStatus |= t & 0xff8000;

					dmaStatus &= ~(t & 0x7f000000);

					extern void IRQ(int irq);
					if ((dmaStatus & 0x8000) ||
						((dmaStatus&(1 << 23)) && ((dmaStatus & 0x7f0000) >> 16) & ((dmaStatus & 0x7f000000) >> 24))) {
						dmaStatus |= 0x80000000;
						//IRQ(3);
					}
				} else {
					printf("W Unimplemented DMA address 0x%08x\n", address);
				}
				return;
			}
			
			address += 0x80;

			if (channel == 2) { // GPU, channel 2\7
				writeDma2(address, data);
			} else if (channel == 6) { // reverse clear OT, channel 6
				writeDma6(address, data);
			}
			else {
				printf("Unimplemented DMA channel %d write \n", channel);
				return;
			}
		}

		void DMA::writeDma2(uint32_t address, uint8_t data)
		{
			mips::CPU *cpu = (mips::CPU*)_cpu;

			if (address >= 0xa0 && address < 0xa4) {
				dma2Address._reg &= ~(0xff << ((address - 0xa0) * 8));
				dma2Address._reg |= data << ((address - 0xa0) * 8);
			}
			if (address >= 0xa4 && address < 0xa8) {
				dma2Count &= ~(0xff << ((address - 0xa4) * 8));
				dma2Count |= data << ((address - 0xa4) * 8);
			}
			if (address >= 0xa8 && address < 0xac) {
				dma2Control &= ~(0xff << ((address - 0xa8) * 8));
				dma2Control |= data << ((address - 0xa8) * 8);

				int mode = (dma2Control & 0x600) >> 9;

				if (dma2Control & (1 << 24)) {
					int addr = dma2Address.address;

					if (mode == 2) { // linked list 
						//printf("DMA2 linked list @ 0x%08x\n", dma2Address);

						for (;;) {
							uint32_t blockInfo = cpu->readMemory32(addr);
							int commandCount = blockInfo >> 24;

							addr += 4;
							for (int i = 0; i < commandCount; i++)	{
								//uint32_t command = readMemory32(addr);
								//writeMemory32(0x1F801810, command);
								//gpu->write(0x10, command);
								gpu->write(0, cpu->readMemory8(addr++));
								gpu->write(0, cpu->readMemory8(addr++));
								gpu->write(0, cpu->readMemory8(addr++));
								gpu->write(0, cpu->readMemory8(addr++));
							}
							if (blockInfo & 0xffffff == 0xffffff) break;
							addr = blockInfo & 0xffffff;
						}
					}
					else if (mode == 1)	{
						int blockSize = dma2Count & 0xffff;
						int blockCount = dma2Count >> 16;
						if (blockCount == 0) blockCount = 0x10000;

						if (dma2Control == 0x01000200) // VRAM READ
						{
							printf("DMA2 VRAM -> CPU @ 0x%08x, BS: 0x%04x, BC: 0x%04x\n", dma2Address, blockSize, blockCount);

							for (int block = 0; block < blockCount; block++){
								for (int i = 0; i < blockSize; i++) {
									//writeMemory32(0x1f801810, readMemory32(addr));
									cpu->writeMemory8(addr++, gpu->read(0));
									cpu->writeMemory8(addr++, gpu->read(1));
									cpu->writeMemory8(addr++, gpu->read(2));
									cpu->writeMemory8(addr++, gpu->read(3));
								}
							}
						}
						else if (dma2Control == 0x01000201) // VRAM WRITE
						{
							printf("DMA2 CPU -> VRAM @ 0x%08x, BS: 0x%04x, BC: 0x%04x\n", dma2Address, blockSize, blockCount);

							for (int block = 0; block < blockCount; block++){
								for (int i = 0; i < blockSize; i++) {
									//writeMemory32(0x1f801810, readMemory32(addr));
									gpu->write(0, cpu->readMemory8(addr++));
									gpu->write(0, cpu->readMemory8(addr++));
									gpu->write(0, cpu->readMemory8(addr++));
									gpu->write(0, cpu->readMemory8(addr++));
								}
							}
						}
						else
						{
							printf("Unsupported cpu to vram dma mode\n");
							__debugbreak();
						}
					}
					else
					{
						printf("Unsupported DMA mode\n");
						__debugbreak();
					}
					dma2Control &= ~(1 << 24); // complete
				}
			}
		}

		void DMA::writeDma6(uint32_t address, uint8_t data)
		{
			mips::CPU *cpu = (mips::CPU*)_cpu;

			if (address >= 0xe0 && address < 0xe4) {
				dma6Address._byte[address - 0xe0] = data;
			} else if (address >= 0xe4 && address < 0xe8) {
				dma6Count._byte[address - 0xe4] = data;
			} else if (address >= 0xe8 && address < 0xec) {
				dma6Control._byte[address - 0xe8] = data;

				if (dma6Control.enabled == CHCR::Enabled::start) {
					dma6Control.startTrigger = CHCR::StartTrigger::clear;
					//printf("DMA6 linked list @ 0x%08x\n", dma6Address);

					int addr = dma6Address.address;
					for (int i = 0; i < dma6Count.syncMode0.wordCount; i++)	{
						if (i == dma6Count.syncMode0.wordCount - 1) cpu->writeMemory32(addr, 0xffffff);
						else cpu->writeMemory32(addr, (addr - 4) & 0xffffff);
						addr -= 4;
					}
					dma6Control.enabled = CHCR::Enabled::stop;
				}
			}
		}
	}
}