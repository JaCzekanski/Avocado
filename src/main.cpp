#include <cstdio>
#include <string>
#include <algorithm>
#include <stdint.h>
#include <bitset>
#include "utils/file.h"
#include "utils/string.h"
#include "mipsInstructions.h"
#include <SDL.h>

#undef main

/*
KUSEG     KSEG0     KSEG1
00000000h 80000000h A0000000h  2048K  Main RAM(first 64K reserved for BIOS)
1F000000h 9F000000h BF000000h  8192K  Expansion Region 1 (ROM / RAM)
1F800000h 9F800000h    --      1K     Scratchpad(D - Cache used as Fast RAM)
1F801000h 9F801000h BF801000h  8K     I / O Ports
1F802000h 9F802000h BF802000h  8K     Expansion Region 2 (I / O Ports)
1FA00000h 9FA00000h BFA00000h  2048K  Expansion Region 3 (whatever purpose)
1FC00000h 9FC00000h BFC00000h  512K   BIOS ROM(Kernel) (4096K max)
       FFFE0000h(KSEG2)        0.5K   I / O Ports(Cache Control)
*/

uint8_t bios[512 * 1024]; //std::vector<uint8_t> bios;
uint8_t ram[2 * 1024 * 1024]; //std::vector<uint8_t> ram;
uint8_t scratchpad[1024]; //std::vector<uint8_t> scratchpad;
uint8_t io[8 * 1024]; //std::vector<uint8_t> io;
uint8_t expansion[0x10000]; //std::vector<uint8_t> expansion;

// r0      zero  - always return 0
// r1      at    - assembler temporary, reserved for use by assembler
// r2-r3   v0-v1 - (value) returned by subroutine
// r4-r7   a0-a3 - (arguments) first four parameters for a subroutine
// r8-r15  t0-t7 - (temporaries) subroutines may use without saving
// r24-r25 t8-t9
// r16-r23 s0-s7 - subroutine regiver variables; a subroutine which will write one of these must save the old value and restore it before it exits, so the calling routine sees their values preserved/
// r26-r27 k0-k1 - Reserved for use by interrupt/trap handler - may change under your feet
// r28     gp    - global pointer - some runtime system maintain this to give easy access to static or extern variables
// r29     sp    - stack pointer
// r30     fp    - frame pointer
// r31     ra    - return address for subroutine
std::string regNames[] = {
	"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
	"t0",   "t1", "t2", "t3", "t4", "t5", "t6", "t7",
	"s0",   "s1", "s2", "s3", "s4", "s5", "s6", "s7",
	"t8",   "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};


bool IsC = false;
uint8_t POST = 0;
bool disassemblyEnabled = false;
bool memoryAccessLogging = true;

std::string part = "";

uint8_t readMemory(uint32_t address)
{
	uint32_t _address = address;
	uint32_t data = 0;
	if (address >= 0xa0000000) address -= 0xa0000000;
	if (address >= 0x80000000) address -= 0x80000000;

	part = "";

	// RAM
	if (address >= 0x00000000 &&
		address <=   0x1fffff)
	{
		data = ram[address];
		if (address == 0x0000b9b0) data = 1;// DUART enable
		//printf("RAM_R: 0x%02x (0x%08x) - 0x%02x\n", address, _address, data);
	}

	// Expansion port (mirrored?)
	else if (address >= 0x1f000000 &&
		address <= 0x1f00ffff)
	{
		address -= 0x1f000000;
		data = expansion[address];
		part = "EXPANSION";
	}

	// Scratch Pad
	else if (address >= 0x1f800000 &&
			address <= 0x1f8003ff)
	{
		address -= 0x1f800000;
		data = scratchpad[address];
		//part = "  SCRATCH";
	}

	// IO Ports
	else if (address >= 0x1f801000 &&
			address <= 0x1f803000)
	{
		address -= 0x1f801000;
		data = io[address];
		if (address >= 0x0000 && address <= 0x0024) part = " MEMCTRL1";
		else if (address >= 0x0040 && address <= 0x005F) part = "   PERIPH";
		else if (address >= 0x0060 && address <= 0x0063) part = " MEMCTRL2";
		else if (address >= 0x0070 && address <= 0x0077) part = "  INTCTRL";
		else if (address >= 0x0080 && address <= 0x00FF) part = "      DMA";
		else if (address >= 0x0100 && address <= 0x012F) part = "    TIMER";
		else if (address >= 0x0800 && address <= 0x0803) part = "    CDROM";
		else if (address >= 0x0810 && address <= 0x0818) part = "      GPU";
		else if (address >= 0x0820 && address <= 0x0828) part = "     MDEC";
		else if (address >= 0x0C00 && address <= 0x0FFF) part = "      SPU";
		else if (address >= 0x1020 && address <= 0x102f) {
			//part = "    DUART";
			if (address == 0x1021) data = 0x0c;
		}
		else part = "       IO";
	}

	else if (address >= 0x1fc00000 && 
			address <= 0x1fc00000 + 512*1024)
	{
		address -= 0x1fc00000;
		data = bios[address];
	}

	else if (_address >= 0xfffe0000 &&
		_address <= 0xfffe0200)
	{
		address = _address - 0xfffe0000;
		data = 0;
		part = "    CACHE";
	}

	else
	{
		printf("READ: Access to unmapped address: 0x%08x (0x%08x)\n", address, _address);
		data = 0;
	}
	return data;
}

// Word - 32bit
// Half - 16bit
// Byte - 8bit

uint8_t readMemory8(uint32_t address)
{
	// TODO: Check address align
	uint8_t data = readMemory(address);
	if (!part.empty() && memoryAccessLogging) printf("%s_R08: 0x%08x - 0x%02x '%c'\n", part.c_str(), address, data, data);
	return data;
}

uint16_t readMemory16(uint32_t address)
{
	// TODO: Check address align
	uint16_t data = 0;
	data |= readMemory(address + 0);
	data |= readMemory(address + 1) << 8;
	if (!part.empty() && memoryAccessLogging) printf("%s_R16: 0x%08x - 0x%04x\n", part.c_str(), address, data);
	return data;
}

uint32_t readMemory32(uint32_t address)
{
	// TODO: Check address align
	uint32_t data = 0;
	data |= readMemory(address + 0);
	data |= readMemory(address + 1) << 8;
	data |= readMemory(address + 2) << 16;
	data |= readMemory(address + 3) << 24;
	if (!part.empty() && memoryAccessLogging) printf("%s_R32: 0x%08x - 0x%08x\n", part.c_str(), address, data);
	return data;
}

void writeMemory( uint32_t address, uint8_t data )
{
	uint32_t _address = address;
	if (address >= 0xa0000000) address -= 0xa0000000;
	if (address >= 0x80000000) address -= 0x80000000;

	part = "";

	// RAM
	if (address >= 0x00000000 &&
		address <= 0x1fffff)
	{
		if (!IsC) {
			ram[address] = data;
			if (disassemblyEnabled) part = "      RAM";
		}
		//printf("RAM_W: 0x%02x (0x%08x) - 0x%02x\n", address, _address, data);
	}

	// Expansion port (mirrored?)
	else if (address >= 0x1f000000 &&
		address <= 0x1f00ffff)
	{
		address -= 0x1f000000;
		expansion[address] = data;
		part = "EXPANSION";
	}

	// Scratch Pad
	else if (address >= 0x1f800000 &&
		address <= 0x1f8003ff)
	{
		address -= 0x1f800000;
		scratchpad[address] = data;
		//part = "  SCRATCH";
	}

	// IO Ports
	else if (address >= 0x1f801000 &&
			address <= 0x1f803000)
	{
		address -= 0x1f801000;

		if (address == 0x1041) part = "     POST";
		else if (address >= 0x0000 && address <= 0x0024) part = " MEMCTRL1";
		else if (address >= 0x0040 && address <= 0x005F) part = "   PERIPH";
		else if (address >= 0x0060 && address <= 0x0063) part = " MEMCTRL2";
		else if (address >= 0x0070 && address <= 0x0077) part = "  INTCTRL";
		else if (address >= 0x0080 && address <= 0x00FF) part = "      DMA";
		else if (address >= 0x0100 && address <= 0x012F) part = "    TIMER";
		else if (address >= 0x0800 && address <= 0x0803) part = "    CDROM";
		else if (address >= 0x0810 && address <= 0x0818) part = "      GPU";
		else if (address >= 0x0820 && address <= 0x0828) part = "     MDEC";
		else if (address >= 0x0C00 && address <= 0x0FFF) part = "      SPU";
		else if (address >= 0x1020 && address <= 0x102f) {
			//part = "    DUART";
			if (address == 0x1023) printf("%c", data);
		}
		else part = "       IO";
	}

	else if (address >= 0x1fc00000 &&
		address <= 0x1fc00000 + 512*1024)
	{
		address -= 0x1fc00000;
		printf("Write to readonly address (BIOS): 0x%08x\n", address);
	}

	else if (_address >= 0xfffe0000 &&
			_address <= 0xfffe0200)
	{
		address = _address - 0xfffe0000;
		part = "    CACHE";
	}

	else
	{
		printf("WRITE: Access to unmapped address: 0x%08x (0x%08x)\n", address, _address);
		data = 0;
	}
}

void writeMemory8(uint32_t address, uint8_t data)
{
	// TODO: Check address align
	writeMemory(address, data);
	if (!part.empty() && memoryAccessLogging) printf("%s_W08: 0x%08x - 0x%02x '%c'\n", part.c_str(), address, data, data);
}

void writeMemory16(uint32_t address, uint16_t data)
{
	// TODO: Check address align
	writeMemory(address + 0, data & 0xff);
	writeMemory(address + 1, data >> 8);
	if (!part.empty() && memoryAccessLogging) printf("%s_W16: 0x%08x - 0x%04x\n", part.c_str(), address, data);
}

void writeMemory32(uint32_t address, uint32_t data)
{
	// TODO: Check address align
	writeMemory(address + 0, data);
	writeMemory(address + 1, data >> 8);
	writeMemory(address + 2, data >> 16);
	writeMemory(address + 3, data >> 24);
	if (!part.empty() && memoryAccessLogging) printf("%s_W32: 0x%08x - 0x%08x\n", part.c_str(), address, data);
}


std::string _mnemonic = "";
std::string _disasm = "";
std::string _pseudo = "";


void executeInstruction(CPU *cpu, Opcode i)
{
	_pseudo = "";
	uint32_t addr = 0;

	cpu->isJumpCycle = true;
	
	auto op = OpcodeTable[i.op];
	op.instruction(cpu, i);

	_mnemonic = op.mnemnic;

	if (disassemblyEnabled) {
		if (_disasm.empty()) printf("%s\n", _mnemonic.c_str());
		else {
			if (!_pseudo.empty())
				printf("   0x%08x  %08x:    %s\n", cpu->PC - 4, readMemory32(cpu->PC - 4), _pseudo.c_str());
			else
				printf("   0x%08x  %08x:    %s\n", cpu->PC - 4, readMemory32(cpu->PC - 4), _disasm.c_str());

			//std::transform(_mnemonic.begin(), _mnemonic.end(), _mnemonic.begin(), ::tolower);
			//printf("%08X %08X %-7s %s\n", cpu->PC - 4, readMemory32(cpu->PC - 4), _mnemonic.c_str(), _disasm.c_str());
		}
	}

	if (cpu->shouldJump && cpu->isJumpCycle) {
		cpu->PC = cpu->jumpPC & 0xFFFFFFFC;
		cpu->jumpPC = 0;
		cpu->shouldJump = false;
	}
}

std::string getStringFromRam(uint32_t addr)
{
	std::string format;

	int c;
	int i = 0;
	while (1)
	{
		c = readMemory(addr);
		addr++;
		if (!c) break;
		format += c;
		if (i++ > 100) break;
	}
	return format;
}

bool doDump = false;
int main( int argc, char** argv )
{
	CPU cpu;
	bool cpuRunning = true;
	memset(cpu.reg, 0, sizeof(uint32_t) * 32);
	
	std::string biosPath = "data/bios/SCPH1000.bin";
	
	auto _bios = getFileContents(biosPath);
	if (_bios.empty()) {
		printf("Cannot open BIOS");
		return 1;
	}
	memcpy(bios, &_bios[0], _bios.size());

	// Pre boot hook
	//uint32_t preBootHookAddress = 0x1F000100;
	//std::string license = "Licensed by Sony Computer Entertainment Inc.";
	//memcpy(&expansion[0x84], license.c_str(), license.size());

	int cycles = 0;
	int frames = 0;
	Opcode _opcode;

	while (cpuRunning)
	{
		_opcode.opcode = readMemory32(cpu.PC);
		//if (cpu.PC == 0xbfc018d0)
		//{
		//	disassemblyEnabled = true;
		//	__debugbreak();
		//	std::string format = getStringFromRam(cpu.reg[4]);
		//	printf("___%s\n", format.c_str());
		//}

		cpu.PC += 4;

		executeInstruction(&cpu, _opcode);

		cycles += 2;

		if (cycles >= 564480) {
			cycles = 0;
			frames++;
			//printf("Frame: %d\n", frames);
		}

		if (doDump)
		{
			std::vector<uint8_t> ramdump;
			ramdump.resize(2 * 1024 * 1024);
			memcpy(&ramdump[0], ram, ramdump.size());
			putFileContents("ram.bin", ramdump);
			doDump = false;
		}
		if (cpu.halted)
		{
			printf("CPU Halted\n");
			//printf("Unknown instruction @ 0x%08x: 0x%08x (copy: %02x %02x %02x %02x)\n", cpu.PC - 4, _opcode.opcode, _opcode.opcode & 0xff, (_opcode.opcode >> 8) & 0xff, (_opcode.opcode >> 16) & 0xff, (_opcode.opcode >> 24) & 0xff);
			cpuRunning = false;
		}
	}

	return 0;
}