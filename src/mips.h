#pragma once
#include <stdint.h>

namespace mips
{
	/*
	Based on http://problemkaputt.de/psx-spx.htm
	0x00000000 - 0x20000000 is cache enabled
	0x80000000 is mirrored to 0x00000000 (with cache)
	0xA0000000 is mirrored to 0x00000000 (without cache)

	00000000 - 00200000 2048K  RAM
	1F000000 - 1F800000 8192K  Expansion ROM
	1F800000 - 1F800400 1K     Scratchpad
	1F801000 - 1F803000 8K     I/O
	1F802000 - 1F804000 8K     Expansion 2
	1FA00000 - 1FC00000 2048K  Expansion 3
	1FC00000 - 1FC80000 512K   BIOS ROM
	FFFE0000 - FFFE0100 0.5K   I/O (Cache Control)
	*/
	/*
	r0      zero  - always 0
	r1      at    - assembler temporary, reserved for use by assembler
	r2-r3   v0-v1 - (value) returned by subroutine
	r4-r7   a0-a3 - (arguments) first four parameters for a subroutine
	r8-r15  t0-t7 - (temporaries) subroutines may use without saving
	r24-r25 t8-t9
	r16-r23 s0-s7 - saved temporaries, must be saved
	r26-r27 k0-k1 - Reserved for use by interrupt/trap handler
	r28     gp    - global pointer - used for accessing static/extern variables
	r29     sp    - stack pointer
	r30     fp    - frame pointer
	r31     ra    - return address
	*/
	struct CPU
	{
		uint32_t PC;
		uint32_t jumpPC;
		bool shouldJump;
		uint32_t reg[32];
		uint32_t COP0[32];
		uint32_t hi, lo;
		bool IsC = false;
		bool halted = false;
		CPU()
		{
			PC = 0xBFC00000;
			jumpPC = 0;
			shouldJump = false;
			for (int i = 0; i < 32; i++) reg[i] = 0;
			for (int i = 0; i < 32; i++) COP0[i] = 0;
			hi = 0;
			lo = 0;
		}

		uint8_t bios[512 * 1024];
		uint8_t ram[2 * 1024 * 1024];
		uint8_t scratchpad[1024];
		uint8_t io[8 * 1024];
		uint8_t expansion[0x10000];

	private:
		uint8_t readMemory(uint32_t address);
		void writeMemory(uint32_t address, uint8_t data);
	public:
		uint8_t readMemory8(uint32_t address);
		uint16_t readMemory16(uint32_t address);
		uint32_t readMemory32(uint32_t address);
		void writeMemory8(uint32_t address, uint8_t data);
		void writeMemory16(uint32_t address, uint16_t data);
		void writeMemory32(uint32_t address, uint32_t data);

		bool executeInstructions( int count );
	};
};