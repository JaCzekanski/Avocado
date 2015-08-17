#pragma once
#include <stdint.h>
#include "device/gpu.h"
#include "device/dma.h"
#include "device/dummy.h"

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

			memoryControl = new device::Dummy("MemCtrl", 0x1f801000);
			joypad = new device::Dummy("Joypad", 0x1f801040);
			serial = new device::Dummy("Serial", 0x1f801050);
			interrupt = new device::Dummy("Interrupt", 0x1f801070);
			dma = new device::dma::DMA();
			dma->setCPU(this);
			timer0 = new device::Dummy("Timer0", 0x1f801100);
			timer1 = new device::Dummy("Timer1", 0x1f801110);
			timer2 = new device::Dummy("Timer2", 0x1f801120);
			cdrom = new device::Dummy("CDROM", 0x1f801800);
			mdec = new device::Dummy("MDEC", 0x1f801820);
			spu = new device::Dummy("SPU", 0x1f801c00, false);
			expansion2 = new device::Dummy("Expansion2", 0x1f802000, false);
		}

		uint8_t bios[512 * 1024];
		uint8_t ram[2 * 1024 * 1024];
		uint8_t scratchpad[1024];
		uint8_t io[8 * 1024];
		uint8_t expansion[0x10000];

	private:
		// Devices
		device::Dummy *memoryControl = nullptr;
		device::Dummy *joypad = nullptr;
		device::Dummy *serial = nullptr;
		device::Dummy *interrupt = nullptr;
		device::dma::DMA *dma = nullptr;
		device::Dummy *timer0 = nullptr;
		device::Dummy *timer1 = nullptr;
		device::Dummy *timer2 = nullptr;
		device::Dummy *cdrom = nullptr;
		device::gpu::GPU *gpu = nullptr;
		device::Dummy *mdec = nullptr;
		device::Dummy *spu = nullptr;
		device::Dummy *expansion2 = nullptr;

		uint8_t readMemory(uint32_t address);
		void writeMemory(uint32_t address, uint8_t data);
	public:
		void setGPU(device::gpu::GPU *gpu) {
			this->gpu = gpu;
			dma->setGPU(gpu);
		}

		uint8_t readMemory8(uint32_t address);
		uint16_t readMemory16(uint32_t address);
		uint32_t readMemory32(uint32_t address);
		void writeMemory8(uint32_t address, uint8_t data);
		void writeMemory16(uint32_t address, uint16_t data);
		void writeMemory32(uint32_t address, uint32_t data);

		bool executeInstructions( int count );
	};
};