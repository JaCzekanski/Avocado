#include "mips.h"
#include "mipsInstructions.h"
#include <cstdio>
#include <string>
#include <deque>

extern bool disassemblyEnabled;
extern bool memoryAccessLogging;
extern char* _mnemonic;
extern std::string _disasm;

namespace mips
{
	int part = 0;

	const char* regNames[] = {
		"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
		"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
		"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
		"t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
	};

	char* memoryRegion[] = {
		"",          // 0
		"RAM",       // 1
		"Expansion1",
		"Scratchpad",
		"MemoryCtrl1",
		"Joypad",    // 5
		"Serial",
		"MemoryCtrl2",
		"Interrupt",
		"DMA",
		"Timer0",    // 10
		"Timer1",
		"Timer2",
		"CDROM",
		"GPU",
		"MDEC",      // 15
		"SPUVoice",
		"SPUControl",
		"SPUReverb",
		"SPUInternal",
		"DUART",    // 20
		"POST",
		"BIOS",
		"MemoryCtrl3",
		"Unknown"
	};

	static uint32_t dma2Control = 0;
	static int dma2Timer = 0;

	static uint32_t dma6Control = 0;
	static int dma6Timer = 0;

	static int htimer = 0;

	static int CDROM_index = 0;
	static std::deque<uint8_t> CDROM_params;
	static std::deque<uint8_t> CDROM_response;
	static std::deque<uint8_t> CDROM_interrupt;

	uint8_t CPU::readMemory(uint32_t address)
	{
		if (address >= 0xfffe0130 && address < 0xfffe0134)
		{
			part = 23;
			return 0;
		}

		if (address >= 0xa0000000) address -= 0xa0000000;
		if (address >= 0x80000000) address -= 0x80000000;

		// RAM
		if (address < 0x200000*4)
		{
			part = 1;
			if (address == 0x0000b9b0) return 1;// DUART enable
			return ram[address % (2 * 1024 * 1024)];
		}

		// Expansion port (mirrored?)
		if (address >= 0x1f000000 && address < 0x1f010000)
		{
			part = 2;
			return expansion[address - 0x1f000000];
		}

		// Scratch Pad
		if (address >= 0x1f800000 && address < 0x1f800400)
		{
			part = 3;
			return scratchpad[address - 0x1f800000];
		}

		// IO Ports
		if (address >= 0x1f801000 && address <= 0x1f803000)
		{
			address -= 0x1f801000;
			if (address >= 0x0000 && address <= 0x0024) part = 4;
			else if (address >= 0x0040 && address <  0x0050) part = 5;
			else if (address >= 0x0050 && address < 0x0060) {
				part = 6;
				if (address == 0x54) return 5;
			}
			else if (address >= 0x0060 && address <  0x0064) part = 7;
			else if (address >= 0x0070 && address <  0x0078) part = 8;
			else if (address >= 0x0080 && address < 0x0100) { // DMA
				part = 9;
				if (address >= 0xa8 && address < 0xac) {
					return dma2Control >> ((address - 0xa8) * 8);
				}
				if (address >= 0xe8 && address < 0xec) {
					return dma6Control >> ((address - 0xe8) * 8);
				}
			}
			else if (address >= 0x0100 && address <  0x0130) {
				if (address >= 0x100 && address < 0x110) part = 10;
				else if (address >= 0x110 && address < 0x120) {
					htimer++;
					if (address < 0x114) return htimer >> ((address - 0x114) * 8);
					part = 11;
				}
				else if (address >= 0x120 && address < 0x130) part = 12;
			}
			else if (address >= 0x0800 && address < 0x0804) {
				if (address == 0x800) {
					uint8_t status = CDROM_index;
					//status |= (!CDROM_data.emprt()) << 6;
					status |= (!CDROM_response.empty()) << 5;
					status |= (CDROM_params.size() >= 16) << 4;
					status |= (CDROM_params.empty()) << 3;

					return status;
				}
				if (address == 0x801) {
					uint8_t response = 0;
					if (!CDROM_response.empty()) {
						response = CDROM_response.front();
						CDROM_response.pop_front();
					}
					return response;
				}
				if (address == 0x803) { // type of response received
					uint8_t status = 0xe0;
					if (!CDROM_interrupt.empty()) {
						status |= CDROM_interrupt.front();
						CDROM_interrupt.pop_front();
					}

					return status;
				}
				part = 13;
			}
			else if (address >= 0x0810 && address < 0x0818) {
				part = 14;
				return gpu->read(address - 0x0810);
			}
			else if (address >= 0x0820 && address <  0x0828) part = 15;
			else if (address >= 0x0C00 && address <  0x0D80) part = 16;
			else if (address >= 0x0D80 && address <  0x0DC0) part = 17;
			else if (address >= 0x0DC0 && address <  0x0E00) part = 18;
			else if (address >= 0x0E00 && address <  0x1000) part = 19;
			else if (address >= 0x1020 && address <  0x1030) {
				//part = 20;
				part = 0;
				if (address == 0x1021) return 0x0c;
			}
			else if (address == 0x1041) part = 21;
			else part = 24;

			return io[address];
		}

		if (address >= 0x1fc00000 && address <= 0x1fc80000)
		{
			//part = 22; 
			part = 0;
			return bios[address - 0x1fc00000];
		}

		part = 24;
		return 0;
	}

	void CPU::writeMemory(uint32_t address, uint8_t data)
	{
		// Cache control
		if (address >= 0xfffe0130 && address < 0xfffe0134)
		{
			part = 23;
			return;
		}

		if (address >= 0xa0000000) address -= 0xa0000000;
		if (address >= 0x80000000) address -= 0x80000000;

		// RAM
		if (address < 0x200000*4)
		{
			part = 1;
			if (!IsC) ram[address%(2*1024*1024)] = data;
		}

		// Expansion port (mirrored?)
		else if (address >= 0x1f000000 && address < 0x1f010000)
		{
			part = 2;
			expansion[address - 0x1f000000] = data;
		}

		// Scratch Pad
		else if (address >= 0x1f800000 && address < 0x1f800400)
		{
			part = 3;
			scratchpad[address - 0x1f800000] = data;
		}

		// IO Ports
		else if (address >= 0x1f801000 && address <= 0x1f803000)
		{
			address -= 0x1f801000;
			if (address >= 0x0000 && address <= 0x0024) part = 4;
			else if (address >= 0x0040 && address <  0x0050) part = 5;
			else if (address >= 0x0050 && address <  0x0060) part = 6;
			else if (address >= 0x0060 && address <  0x0064) part = 7;
			else if (address >= 0x0070 && address <  0x0078) part = 8;
			else if (address >= 0x0080 && address <= 0x0100) { // DMA
				part = 9;
				if (address >= 0xa0 && address < 0xb0) { // GPU, channel 2
					static uint32_t dma2Address = 0;
					static uint32_t dma2Count = 0;


					if (address >= 0xa0 && address < 0xa4) {
						dma2Address &= ~(0xff << ((address - 0xa0) * 8));
						dma2Address |= data << ((address - 0xa0) * 8);

						dma2Address &= 0x1FFFFFF;
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
							int addr = dma2Address;

							if (mode == 2) { // linked list 
								printf("DMA2 linked list @ 0x%08x\n", dma2Address);

								for (;;) {
									uint32_t blockInfo = readMemory32(addr);
									int commandCount = blockInfo >> 24;

									addr += 4;
									for (int i = 0; i < commandCount; i++)	{
										//uint32_t command = readMemory32(addr);
										//writeMemory32(0x1F801810, command);
										//gpu->write(0x10, command);
										gpu->write(0, readMemory8(addr++));
										gpu->write(0, readMemory8(addr++));
										gpu->write(0, readMemory8(addr++));
										gpu->write(0, readMemory8(addr++));
									}
									if (blockInfo & 0xffffff == 0xffffff) break;
									addr = blockInfo & 0xffffff;
								}
							}
							else if (mode == 1)	{
								int blockSize = dma2Count & 0xffff;
								int blockCount = dma2Count >> 16;
								if (blockCount == 0) blockCount = 0x10000;

								if (dma2Control == 0x01000200 ) // VRAM READ
								{
									printf("DMA2 VRAM -> CPU @ 0x%08x, BS: 0x%04x, BC: 0x%04x\n", dma2Address, blockSize, blockCount);

									for (int block = 0; block < blockCount; block++){
										for (int i = 0; i < blockSize; i++) {
											//writeMemory32(0x1f801810, readMemory32(addr));
											writeMemory8(addr++, gpu->read(0));
											writeMemory8(addr++, gpu->read(1));
											writeMemory8(addr++, gpu->read(2));
											writeMemory8(addr++, gpu->read(3));
										}
									}
								}
								else if (dma2Control == 0x01000201) // VRAM WRITE
								{
									printf("DMA2 CPU -> VRAM @ 0x%08x, BS: 0x%04x, BC: 0x%04x\n", dma2Address, blockSize, blockCount);

									for (int block = 0; block < blockCount; block++){
										for (int i = 0; i < blockSize; i++) {
											//writeMemory32(0x1f801810, readMemory32(addr));
											gpu->write(0, readMemory8(addr++));
											gpu->write(0, readMemory8(addr++));
											gpu->write(0, readMemory8(addr++));
											gpu->write(0, readMemory8(addr++));
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
							extern void IRQ(int irq);

							if (readMemory32(0x1f8010f4)&(1 << 16 + 2)) {
								writeMemory32(0x1f8010f4, readMemory32(0x1f8010f4) | (1 << 24 + 2));
								//IRQ(3);
							}
						}
					}
				}
				else if (address >= 0xe0 && address < 0xf0) { // reverse clear OT, channel 6
					static uint32_t dma6Address = 0; 
					static uint32_t dma6Count = 0;


					if (address >= 0xe0 && address < 0xe4) {
						dma6Address &= ~(0xff << ((address - 0xe0) * 8));
						dma6Address |=   data << ((address - 0xe0) * 8);

						dma6Address &= 0x1FFFFFF;
					}
					if (address >= 0xe4 && address < 0xe8) {
						dma6Count &= ~(0xff << ((address - 0xe4) * 8));
						dma6Count |=   data << ((address - 0xe4) * 8);
					}
					if (address >= 0xe8 && address < 0xec) {
						dma6Control &= ~(0xff << ((address - 0xe8) * 8));
						dma6Control |=   data << ((address - 0xe8) * 8);

						if (dma6Control & (1 << 24)) {
							dma6Control &= ~(1 << 28); // started
							printf("DMA6 linked list @ 0x%08x\n", dma6Address);
							int addr = dma6Address;
							for (int i = 0; i < dma6Count; i++)	{
								if (i == dma6Count - 1) writeMemory32(addr, 0xffffff);
								else writeMemory32(addr, (addr - 4) & 0xffffff);
								addr -= 4;
							}
							dma6Control &= ~(1 << 24); // complete

							extern void IRQ(int irq);
							if (readMemory32(0x1f8010f4)&(1 << 16 + 6)) {
								writeMemory32(0x1f8010f4, readMemory32(0x1f8010f4) | (1 << 24 + 6));
								//IRQ(3);
							}
						}
					}
				}
			}
			else if (address >= 0x0100 && address <= 0x012F) {
				if (address >= 0x100 && address < 0x110) part = 10;
				else if (address >= 0x110 && address < 0x120) part = 11;
				else if (address >= 0x120 && address < 0x130) part = 12;
			}
			else if (address >= 0x0800 && address <= 0x0803) {
				part = 13;
				if (address == 0x800) CDROM_index = data & 3;
				if (address == 0x801) { // Command register
					CDROM_interrupt.clear();
					CDROM_response.clear();
					if (CDROM_index == 0) {
						if (data == 0x0a) // Init 
						{
							CDROM_interrupt.push_back(3);
							CDROM_response.push_back(0x02); //stat

							CDROM_interrupt.push_back(2);
							CDROM_response.push_back(0x02); //stat
						}
						if (data == 0x19) // Test
						{
							if (CDROM_params.front() == 0x20) // Get CDROM BIOS date/version (yy,mm,dd,ver)
							{
								CDROM_params.pop_front();
								CDROM_interrupt.push_back(3);
								CDROM_response.push_back(0x97);
								CDROM_response.push_back(0x01);
								CDROM_response.push_back(0x10);
								CDROM_response.push_back(0xc2);
							}
						}
					}
					CDROM_params.clear();
				}
				if (address == 0x802) { // Parameter fifo
					if (CDROM_index == 0) {
						CDROM_params.push_back(data);
					}
				}
			}
			else if (address >= 0x0810 && address <= 0x0818) {
				part = 14;
				gpu->write(address - 0x0810, data);
			}
			else if (address >= 0x0820 && address <= 0x0828) part = 15;
			else if (address >= 0x0C00 && address <  0x0D80) part = 16;
			else if (address >= 0x0D80 && address <  0x0DC0) part = 17;
			else if (address >= 0x0DC0 && address <  0x0E00) part = 18;
			else if (address >= 0x0E00 && address <  0x1000) part = 19;
			else if (address >= 0x1020 && address <= 0x102f) {
				//part = 20;
				part = 0;
				if (address == 0x1023) printf("%c", data);
			}
			else if (address == 0x1041) part = 21;
			else part = 24;

			io[address] = data;
		}

		else part = 24;
	}

	uint8_t CPU::readMemory8(uint32_t address)
	{
		uint8_t data = readMemory(address);
		if (part > 1 && memoryAccessLogging) printf("%9s %12s R08: 0x%08x - 0x%02x\n", "", memoryRegion[part], address, data);
		return data;
	}

	uint16_t CPU::readMemory16(uint32_t address)
	{
		uint16_t data = 0;
		data |= readMemory(address + 0);
		data |= readMemory(address + 1) << 8;
		if (part > 1 && memoryAccessLogging) printf("%9s %12s R16: 0x%08x - 0x%04x\n", (address & 1) ? "Unaligned" : "", memoryRegion[part], address, data);
		return data;
	}

	uint32_t CPU::readMemory32(uint32_t address)
	{
		uint32_t data = 0;
		data |= readMemory(address + 0);
		data |= readMemory(address + 1) << 8;
		data |= readMemory(address + 2) << 16;
		data |= readMemory(address + 3) << 24;
		if (part > 1 && memoryAccessLogging) printf("%9s %12s R32: 0x%08x - 0x%08x\n", (address & 3) ? "Unaligned" : "", memoryRegion[part], address, data);
		return data;
	}


	void CPU::writeMemory8(uint32_t address, uint8_t data)
	{
		writeMemory(address, data);
		if (part > 1 && memoryAccessLogging) printf("%9s %12s W08: 0x%08x - 0x%02x\n", "", memoryRegion[part], address, data);
	}

	void CPU::writeMemory16(uint32_t address, uint16_t data)
	{
		writeMemory(address + 0, data & 0xff);
		writeMemory(address + 1, data >> 8);
		if (part > 1 && memoryAccessLogging) printf("%9s %12s W16: 0x%08x - 0x%04x\n", (address & 1) ? "Unaligned" : "", memoryRegion[part], address, data);
	}

	void CPU::writeMemory32(uint32_t address, uint32_t data)
	{
		writeMemory(address + 0, data);
		writeMemory(address + 1, data >> 8);
		writeMemory(address + 2, data >> 16);
		writeMemory(address + 3, data >> 24);
		if (part > 1 && memoryAccessLogging) printf("%9s %12s W32: 0x%08x - 0x%08x\n", (address & 3) ? "Unaligned" : "", memoryRegion[part], address, data);
	}

	bool CPU::executeInstructions(int count)
	{
		mipsInstructions::Opcode _opcode;
		for (int i = 0; i < count; i++) {
			reg[0] = 0;
			_opcode.opcode = readMemory32(PC);

			bool isJumpCycle = shouldJump;
			const auto &op = mipsInstructions::OpcodeTable[_opcode.op];
			_mnemonic = op.mnemnic;

			op.instruction(this, _opcode);

			if (disassemblyEnabled) {
				printf("   0x%08x  %08x:    %s %s\n", PC, _opcode.opcode, _mnemonic, _disasm.c_str());
			}

			//extern uint32_t memoryDumpAddress;
			//if (memoryDumpAddress != 0)
			//{
			//	for (int y = -10; y < 40; y++)
			//	{
			//		printf("%08x  ", memoryDumpAddress + y * 16);
			//		for (int x = 0; x < 4; x++)
			//		{
			//			uint8_t byte = readMemory8(memoryDumpAddress + y * 16 + x);
			//			printf("%02x ", byte);
			//		}
			//		printf("  ");
			//		for (int x = 0; x < 4; x++)
			//		{
			//			uint8_t byte = readMemory8(memoryDumpAddress + y * 16 + x);
			//			printf("%c", byte>32 ? byte : ' ');
			//		}
			//		printf("\n");
			//	}
			//	__debugbreak();
			//	memoryDumpAddress = 0;
			//}

			if (halted) return false;
			if (isJumpCycle) {
				PC = jumpPC & 0xFFFFFFFC;
				jumpPC = 0;
				shouldJump = false;
			}
			else PC += 4;
		}
		return true;
	}
}