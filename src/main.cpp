#include <cstdio>
#include <string>
#include <stdint.h>
#include <bitset>
#include "utils/file.h"
#include "utils/string.h"
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

std::vector<uint8_t> bios;
std::vector<uint8_t> ram;
std::vector<uint8_t> scratchpad;
std::vector<uint8_t> io;


struct CPU
{
	uint32_t PC = 0xBFC00000;
	uint32_t reg[32];
};

uint32_t readMemory( uint32_t address )
{
	uint32_t data = 0;
	if (address >= 0xa0000000) address -= 0xa0000000;
	if (address >= 0x80000000) address -= 0x80000000;


	// RAM
	if (address >= 0x00000000 &&
		address <=   0x1fffff)
	{
		data = ram[address];
	}

	// Expansion port (mirrored?)
	else if (address >= 0x1f000000 &&
		address <= 0x1f00ffff)
	{
		data = 0;
	}

	// Scratch Pad
	else if (address >= 0x1f800000 &&
			address <= 0x1f8003ff)
	{
		address -= 0x1f800000;
		data = scratchpad[address];
	}

	// IO Ports
	else if (address >= 0x1f801000 &&
			address <= 0x1f803000)
	{
		address -= 0x1f801000;
		data = io[address];
	}


	else if (address >= 0x1fc00000 && 
			address <= 0x1fc00000 + bios.size())
	{
		address -= 0x1fc00000;
		
		data = bios[address] + 
				(bios[address+1] << 8) + 
				(bios[address+2] << 16) + 
				(bios[address+3] << 24);
	}

	else
	{
		printf("Access to unmapped address: 0x%08x\n");
		data = 0;
	}
	return data;
}

// Word - 32bit
// Half - 16bit
// Byte - 8bit

void writeMemory8( uint32_t address, uint8_t data )
{
	if (address >= 0xa0000000) address -= 0xa0000000;
	if (address >= 0x80000000) address -= 0x80000000;

	// RAM
	if (address >= 0x00000000 &&
		address <= 0x1fffff)
	{
		ram[address] = data;
	}

	// Expansion port (mirrored?)
	else if (address >= 0x1f000000 &&
		address <= 0x1f00ffff)
	{
		//data = 0;
	}

	// Scratch Pad
	else if (address >= 0x1f800000 &&
		address <= 0x1f8003ff)
	{
		address -= 0x1f800000;
		scratchpad[address] = data;
	}

	// IO Ports
	else if (address >= 0x1f801000 &&
		address <= 0x1f803000)
	{
		address -= 0x1f801000;
		io[address] = data;
	}

	else if (address >= 0x1fc00000 &&
		address <= 0x1fc00000 + bios.size())
	{
		address -= 0x1fc00000;

		printf("Write to readonly address (BIOS): 0x%08x\n");
	}

	else
	{
		printf("Access to unmapped address: 0x%08x\n");
		data = 0;
	}
}

void writeMemory16(uint32_t address, uint16_t data)
{
	// TODO: Check address align
	writeMemory8(address + 0, data & 0xff);
	writeMemory8(address + 1, data & 0xff00);
}

void writeMemory32(uint32_t address, uint32_t data)
{
	// TODO: Check address align
	writeMemory8(address+0, data & 0xff);
	writeMemory8(address+1, data & 0xff00);
	writeMemory8(address+2, data & 0xff0000);
	writeMemory8(address+3, data & 0xff000000);
}

bool executeInstruction( CPU *cpu, uint32_t instruction )
{
	// I-Type: Immediate
	//    6bit   5bit   5bit          16bit
	//  |  OP  |  rs  |  rt  |        imm         |

	// J-Type: Jump
	//    6bit                26bit
	//  |  OP  |             target               |

	// R-Type: Register
	//    6bit   5bit   5bit   5bit   5bit   6bit
	//  |  OP  |  rs  |  rt  |  rd  |  sh  | fun  |

	// OP - 6bit operation code
	// rs, rt, rd - 5bit source, target, destination register
	// imm - 16bit immediate value
	// target - 26bit jump address
	// sh - 5bit shift amount
	// fun - 6bit function field

	uint8_t op   = (instruction & 0xfc000000) >> 26;
	uint8_t rs   = (instruction &  0x3e00000) >> 21;
	uint8_t rt   = (instruction &   0x1f0000) >> 16;
	uint8_t rd   = (instruction &     0xf800) >> 11;
	uint8_t sh   = (instruction &      0x7c0) >> 6;
	uint8_t fun  = (instruction &       0x3f);

	uint16_t imm    = (instruction & 0x0000ffff);
	uint32_t target = (instruction &  0x3ffffff);

	std::string mnemonic = "";
	std::string _disasm = "";

	uint32_t addr = 0;

#define disasm(fmt, ...) _disasm=string_format(fmt, ##__VA_ARGS__)
#define b(x) std::bitset<6>(x).to_ullong()
	// Jump
	// J target
	if(op == b("000010")) {
		mnemonic = "J";
		disasm("%s 0x%x", mnemonic.c_str(), (target << 2));
		cpu->PC = (cpu->PC&0xf0000000) | (target << 2);
	}

	// Add Immediate Unsigned Word
	// ADDIU rt, rs, imm
	else if(op == b("001001")) {
		mnemonic = "ADDIU";
		disasm("%s r%d, r%d, 0x%x", mnemonic.c_str(), rt, rs, imm);

		cpu->reg[rt] = (cpu->reg[rs] & 0xffff00000) + imm;
	}

	// Or Immediete
	// ORI rt, rs, imm
	else if (op == b("001101")) {
		mnemonic = "ORI";
		disasm("%s r%d, r%d, 0x%x", mnemonic.c_str(), rt, rs, imm);

		cpu->reg[rt] = (cpu->reg[rs] & 0xffff00000) | imm;
	}

	// Load Upper Immediate
	// LUI rt, imm
	else if (op == b("001111")) {
		mnemonic = "LUI";
		disasm("%s r%d, 0x%x", mnemonic.c_str(), rt, imm);

		cpu->reg[rt] = imm << 16;
	}

	// Store Word
	// SW rt, offset(base)
	else if (op == b("101011")) {
		mnemonic = "SW";
		disasm("%s r%d, %d(r%d)", mnemonic.c_str(), rt, imm, rs);
		addr = cpu->reg[rs] + imm;
		writeMemory32(addr, cpu->reg[rt]);
	}

	// R Type
	else if (op == b("000000")) {
		// NOP
		if (fun == b("000000")) {
			mnemonic = "NOP";
		}

		// Or
		// OR rd, rs, rt
		else if (fun == b("100101")) {
			mnemonic = "OR";
			disasm("%s r%d, r%d, r%d", mnemonic.c_str(), rd, rs, rt);

			cpu->reg[rd] = cpu->reg[rs] | cpu->reg[rt];
		}
		else {
			return false;
		}
	}

	else
	{
		return false;
	}

#undef b

	if (_disasm.empty()) printf("%s", mnemonic.c_str());
	else printf("%s", _disasm.c_str());
	return true;
}

int main( int argc, char** argv )
{
	CPU cpu;
	bool cpuRunning = true;
	memset(cpu.reg, 0, sizeof(uint32_t) * 32);

	std::string biosPath = "data/bios/SCPH-102.bin";
	
	bios = getFileContents(biosPath);
	ram.resize(1024 * 1024 * 2);
	scratchpad.resize(1024);
	io.resize(2048);

	while (cpuRunning)
	{
		uint32_t opcode = readMemory(cpu.PC);
		printf("0x%08x: ", cpu.PC);

		cpu.PC += 4;

		bool executed = executeInstruction(&cpu, opcode);

		if (!executed)
		{
			cpuRunning = false;
			printf("Unknown instruction: 0x%08x\n", opcode);
		}
		printf("\n");
	}

	return 0;
}