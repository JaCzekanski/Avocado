#include "mips.h"
#include "mipsInstructions.h"
#include <cstdio>
#include <string>

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
		if (address < 0x200000)
		{
			part = 1;
			if (address == 0x0000b9b0) return 1;// DUART enable
			return ram[address];
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
			else if (address >= 0x0050 && address <  0x0060) part = 6;
			else if (address >= 0x0060 && address <  0x0064) part = 7;
			else if (address >= 0x0070 && address <  0x0078) part = 8;
			else if (address >= 0x0080 && address <  0x0100) part = 9;
			else if (address >= 0x0100 && address <  0x0130) {
				if (address >= 0x100 && address < 0x110) part = 10;
				else if (address >= 0x110 && address < 0x120) part = 11;
				else if (address >= 0x120 && address < 0x130) part = 12;
			}
			else if (address >= 0x0800 && address < 0x0804) part = 13;
			else if (address >= 0x0810 && address < 0x0818) {
				part = 14;
				if (address >= 0x814 && address <= 0x817)
					return (0x1C000000) >> ((address - 0x814) * 8);
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
		if (address < 0x200000)
		{
			part = 1;
			if (!IsC) ram[address] = data;
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
			else if (address >= 0x0060 && address <= 0x0063) part = 7;
			else if (address >= 0x0070 && address <= 0x0077) part = 8;
			else if (address >= 0x0080 && address <= 0x00FF) part = 9;
			else if (address >= 0x0100 && address <= 0x012F) {
				if (address >= 0x100 && address < 0x110) part = 10;
				else if (address >= 0x110 && address < 0x120) part = 11;
				else if (address >= 0x120 && address < 0x130) part = 12;
			}
			else if (address >= 0x0800 && address <= 0x0803) part = 13;
			else if (address >= 0x0810 && address <= 0x0818) part = 14;
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
			_opcode.opcode = readMemory32(PC);

			bool isJumpCycle = shouldJump;
			const auto &op = mipsInstructions::OpcodeTable[_opcode.op];
			_mnemonic = op.mnemnic;

			op.instruction(this, _opcode);

			if (disassemblyEnabled) {
				printf("   0x%08x  %08x:    %s %s\n", PC, _opcode.opcode, _mnemonic, _disasm.c_str());
			}

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