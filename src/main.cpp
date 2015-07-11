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

std::string regNames[] = {
	"zero",
	"at",
	"v0", "v1",
	"a0", "a1", "a2", "a3",
	"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
	"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
	"k0", "k1",
	"gp",
	"sp",
	"fp",
	"ra"
};

struct CPU
{
	uint32_t PC = 0xBFC00000;
	uint32_t jumpPC; 
	bool shouldJump = false;
	uint32_t reg[32];
	uint32_t COP0[32];
	uint32_t hi, lo;
};

bool IsC = false;
uint8_t POST = 0;
bool disassemblyEnabled = false;


uint8_t readMemory8( uint32_t address )
{
	uint32_t _address = address;
	uint32_t data = 0;
	if (address >= 0xa0000000) address -= 0xa0000000;
	if (address >= 0x80000000) address -= 0x80000000;


	// RAM
	if (address >= 0x00000000 &&
		address <=   0x1fffff)
	{
		data = ram[address];
		//printf("RAM_R: 0x%02x (0x%08x) - 0x%02x\n", address, _address, data);
	}

	// Expansion port (mirrored?)
	else if (address >= 0x1f000000 &&
		address <= 0x1f00ffff)
	{
		data = 0;
		printf("EXP_R: 0x%02x (0x%08x) - 0x%02x\n", address, _address, data);
	}

	// Scratch Pad
	else if (address >= 0x1f800000 &&
			address <= 0x1f8003ff)
	{
		address -= 0x1f800000;
		data = scratchpad[address];
		printf("SCRATCH_R: 0x%02x (0x%08x) - 0x%02x\n", address, _address, data);
	}

	// IO Ports
	else if (address >= 0x1f801000 &&
			address <= 0x1f803000)
	{
		address -= 0x1f801000;
		data = io[address];
		printf("IO_R: 0x%02x (0x%08x) - 0x%02x\n", address, _address, data);
	}


	else if (address >= 0x1fc00000 && 
			address <= 0x1fc00000 + bios.size())
	{
		address -= 0x1fc00000;
		
		data = bios[address];
	}

	else if (_address >= 0xfffe0000 &&
		_address <= 0xfffe0200)
	{
		address = _address - 0xfffe0000;
		data = 0;
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

uint16_t readMemory16(uint32_t address)
{
	// TODO: Check address align
	uint16_t data = 0;
	data |= readMemory8(address + 0);
	data |= readMemory8(address + 1) << 8;
	return data;
}

uint32_t readMemory32(uint32_t address)
{
	// TODO: Check address align
	uint32_t data = 0;
	data |= readMemory8(address + 0);
	data |= readMemory8(address + 1) << 8;
	data |= readMemory8(address + 2) << 16;
	data |= readMemory8(address + 3) << 24;
	return data;
}

void writeMemory8( uint32_t address, uint8_t data )
{
	uint32_t _address = address;
	if (address >= 0xa0000000) address -= 0xa0000000;
	if (address >= 0x80000000) address -= 0x80000000;

	// RAM
	if (address >= 0x00000000 &&
		address <= 0x1fffff)
	{
		if (!IsC) ram[address] = data;
		//printf("RAM_W: 0x%02x (0x%08x) - 0x%02x\n", address, _address, data);
	}

	// Expansion port (mirrored?)
	else if (address >= 0x1f000000 &&
		address <= 0x1f00ffff)
	{
		//data = 0;
		printf("EXP_W: 0x%02x (0x%08x) - 0x%02x\n", address, _address, data);
	}

	// Scratch Pad
	else if (address >= 0x1f800000 &&
		address <= 0x1f8003ff)
	{
		address -= 0x1f800000;
		scratchpad[address] = data;
		printf("SCRATCH_W: 0x%02x (0x%08x) - 0x%02x\n", address, _address, data);
	}

	// IO Ports
	else if (address >= 0x1f801000 &&
		address <= 0x1f803000)
	{
		address -= 0x1f801000;
		io[address] = data;
		printf("IO_W: 0x%02x (0x%08x) - 0x%02x\n", address, _address, data);

		if (address == 0x1041) {
			POST = data;
		}
	}

	else if (address >= 0x1fc00000 &&
		address <= 0x1fc00000 + bios.size())
	{
		address -= 0x1fc00000;

		printf("Write to readonly address (BIOS): 0x%08x\n", address);
	}

	else if (_address >= 0xfffe0000 &&
			_address <= 0xfffe0200)
	{
		address = _address - 0xfffe0000;
		//printf("W: Cache Control 0x%03x: 0x%x\n", address, data);
	}

	else
	{
		printf("WRITE: Access to unmapped address: 0x%08x (0x%08x)\n", address, _address);
		data = 0;
	}
}

void writeMemory16(uint32_t address, uint16_t data)
{
	// TODO: Check address align
	writeMemory8(address + 0, data);
	writeMemory8(address + 1, data >> 8);
}

void writeMemory32(uint32_t address, uint32_t data)
{
	// TODO: Check address align
	writeMemory8(address+0, data);
	writeMemory8(address+1, data >> 8);
	writeMemory8(address+2, data >> 16);
	writeMemory8(address+3, data >> 24);
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

	uint16_t imm = (instruction & 0x0000ffff);
	int16_t  offset = (instruction & 0x0000ffff);
	uint32_t target = (instruction &  0x3ffffff);

	std::string mnemonic = "";
	std::string _disasm = "";

	uint32_t addr = 0;

	bool isJumpCycle = true;

	static int nopCounter = 0;

#define disasm(fmt, ...) _disasm=string_format(fmt, ##__VA_ARGS__)
#define b(x) std::bitset<6>(x).to_ullong()
	// Jump
	// J target
	if(op == b("000010")) {
		mnemonic = "J";
		disasm("%s 0x%x", mnemonic.c_str(), (target << 2));
		cpu->shouldJump = true;
		isJumpCycle = false;
		cpu->jumpPC = (cpu->PC & 0xf0000000) | (target << 2);
	}

	// Jump And Link
	// JAL target
	else if (op == b("000011")) {
		mnemonic = "JAL";
		disasm("%s 0x%x", mnemonic.c_str(), (target << 2));
		cpu->shouldJump = true;
		isJumpCycle = false;
		cpu->jumpPC = (cpu->PC & 0xf0000000) | (target << 2);
		cpu->reg[31] = cpu->PC + 4;
	}

	// Branches

	// Branch On Equal
	// BEQ rs, rt, offset
	else if (op == b("000100")) {
		mnemonic = "BEQ";
		disasm("%s r%d, r%d, 0x%x", mnemonic.c_str(), rs, rt, imm);

		if (cpu->reg[rt] == cpu->reg[rs]) {
			cpu->shouldJump = true;
			isJumpCycle = false;
			cpu->jumpPC = (cpu->PC & 0xf0000000) | (cpu->PC) + (offset << 2);
		}
	}

	// Branch On Greater Than Zero
	// BGTZ rs, offset
	else if (op == b("000111")) {
		mnemonic = "BGTZ";
		disasm("%s r%d, 0x%x", mnemonic.c_str(), rs, imm);

		if (cpu->reg[rs] > 0) {
			cpu->shouldJump = true;
			isJumpCycle = false;
			cpu->jumpPC = (cpu->PC & 0xf0000000) | (cpu->PC) + (offset << 2);
		}
	}

	// Branch On Less Than Or Equal To Zero
	// BLEZ rs, offset

	else if (op == b("000110")) {
		mnemonic = "BLEZ";
		disasm("%s r%d, 0x%x", mnemonic.c_str(), rs, imm);

		if (cpu->reg[rs] == 0 || (cpu->reg[rs] & 0x80000000)) {
			cpu->shouldJump = true;
			isJumpCycle = false;
			cpu->jumpPC = (cpu->PC & 0xf0000000) | (cpu->PC) + (offset << 2);
		}
	}


	else if (op == b("000001")) {
		// Branch On Greater Than Or Equal To Zero
		// BGEZ rs, offset
		if (rt == b("00001")) {
			mnemonic = "BGEZ";
			disasm("%s r%d, 0x%x", mnemonic.c_str(), rs, imm);

			if (cpu->reg[rs] >= 0) {
				cpu->shouldJump = true;
				isJumpCycle = false;
				cpu->jumpPC = (cpu->PC & 0xf0000000) | (cpu->PC) + (offset << 2);
			}
		}

		// Branch On Greater Than Or Equal To Zero And Link
		// BGEZAL rs, offset
		else if (rt == b("10001")) {
			mnemonic = "BGEZAL";
			disasm("%s r%d, 0x%x", mnemonic.c_str(), rs, imm);

			cpu->reg[31] = cpu->PC + 4;
			if (cpu->reg[rs] >= 0) {
				cpu->shouldJump = true;
				isJumpCycle = false;
				cpu->jumpPC = (cpu->PC & 0xf0000000) | (cpu->PC) + (offset << 2);
			}
		}

		// Branch On Less Than Zero
		// BLTZ rs, offset
		else if (rt == b("00000")) {
			mnemonic = "BLTZ";
			disasm("%s r%d, 0x%x", mnemonic.c_str(), rs, imm);

			if (((int32_t)cpu->reg[rs]) < 0) {
				cpu->shouldJump = true;
				isJumpCycle = false;
				cpu->jumpPC = (cpu->PC & 0xf0000000) | (cpu->PC) + (offset << 2);
			}
		}

		else return false;
	}

	// Branch On Not Equal
	// BGTZ rs, offset
	else if (op == b("000101")) {
		mnemonic = "BNE";
		disasm("%s r%d, r%d, 0x%x", mnemonic.c_str(), rs, rt, imm);

		if (cpu->reg[rt] != cpu->reg[rs]) {
			cpu->shouldJump = true;
			isJumpCycle = false;
			cpu->jumpPC = (cpu->PC & 0xf0000000) | (cpu->PC) + (offset << 2);
		}
	}

	// And Immediate
	// ANDI rt, rs, imm
	else if (op == b("001100")) {
		mnemonic = "ANDI";
		disasm("%s r%d, r%d, 0x%x", mnemonic.c_str(), rt, rs, imm);

		cpu->reg[rt] = cpu->reg[rs] | imm;
	}

	// Xor Immediate
	// XORI rt, rs, imm
	else if (op == b("001110")) {
		mnemonic = "XORI";
		disasm("%s r%d, r%d, 0x%x", mnemonic.c_str(), rt, rs, imm);

		cpu->reg[rt] = cpu->reg[rs] ^ imm;
	}

	// Add Immediate Unsigned Word
	// ADDIU rt, rs, imm
	else if(op == b("001001")) {
		mnemonic = "ADDIU";
		disasm("%s r%d, r%d, 0x%x", mnemonic.c_str(), rt, rs, offset);

		cpu->reg[rt] = cpu->reg[rs] + offset;
	}

	// Add Immediate Word
	// ADDI rt, rs, imm
	else if (op == b("001000")) {
		mnemonic = "ADDI";
		disasm("%s r%d, r%d, 0x%x", mnemonic.c_str(), rt, rs, offset);

		cpu->reg[rt] = cpu->reg[rs] + offset;
	}

	// Or Immediete
	// ORI rt, rs, imm
	else if (op == b("001101")) {
		mnemonic = "ORI";
		disasm("%s r%d, r%d, 0x%x", mnemonic.c_str(), rt, rs, imm);

		cpu->reg[rt] = (cpu->reg[rs] & 0xffff0000) | imm;
	}

	// Load Upper Immediate
	// LUI rt, imm
	else if (op == b("001111")) {
		mnemonic = "LUI";
		disasm("%s r%d, 0x%x", mnemonic.c_str(), rt, imm);

		cpu->reg[rt] = imm << 16;
	}

	// Load Byte
	// LB rt, offset(base)
	else if (op == b("100000")) {
		mnemonic = "LB";
		disasm("%s r%d, %d(r%d)", mnemonic.c_str(), rt, offset, rs);

		addr = cpu->reg[rs] + offset;
		cpu->reg[rt] = readMemory8(addr);
	}

	// Load Byte Unsigned
	// LBU rt, offset(base)
	else if (op == b("100100")) {
		mnemonic = "LBU";
		disasm("%s r%d, %d(r%d)", mnemonic.c_str(), rt, offset, rs);

		addr = cpu->reg[rs] + offset;
		cpu->reg[rt] = readMemory8(addr);
	}

	// Load Word
	// LW rt, offset(base)
	else if (op == b("100011")) {
		mnemonic = "LW";
		disasm("%s r%d, %d(r%d)", mnemonic.c_str(), rt, offset, rs);

		addr = cpu->reg[rs] + offset;
		cpu->reg[rt] = readMemory32(addr);
	}

	// Store Byte
	// SB rt, offset(base)
	else if (op == b("101000")) {
		mnemonic = "SB";
		disasm("%s r%d, %d(r%d)", mnemonic.c_str(), rt, offset, rs);
		addr = cpu->reg[rs] + offset;
		writeMemory8(addr, cpu->reg[rt]);
	}

	// Store Halfword
	// SH rt, offset(base)
	else if (op == b("101001")) {
		mnemonic = "SW";
		disasm("%s r%d, %d(r%d)", mnemonic.c_str(), rt, offset, rs);
		addr = cpu->reg[rs] + offset;
		writeMemory16(addr, cpu->reg[rt]);
	}

	// Store Word
	// SW rt, offset(base)
	else if (op == b("101011")) {
		mnemonic = "SW";
		disasm("%s r%d, %d(r%d)", mnemonic.c_str(), rt, offset, rs);
		addr = cpu->reg[rs] + offset;
		writeMemory32(addr, cpu->reg[rt]);
	}

	// Set On Less Than Immediate Unsigned
	// SLTI rd, rs, rt
	else if (op == b("001010")) {
		mnemonic = "SLTI";
		disasm("%s r%d, r%d, 0x%x", mnemonic.c_str(), rt, rs, imm);

		if (cpu->reg[rs] < imm) cpu->reg[rt] = 1;
		else cpu->reg[rt] = 0;
	}

	// Set On Less Than Immediate Unsigned
	// SLTIU rd, rs, rt
	else if (op == b("001011")) {
		mnemonic = "SLTIU";
		disasm("%s r%d, r%d, 0x%x", mnemonic.c_str(), rt, rs, imm);

		if (cpu->reg[rs] < imm) cpu->reg[rt] = 1;
		else cpu->reg[rt] = 0;
	}

	// Coprocessor zero
	else if (op == b("010000")) {
		// Move from co-processor zero
		// MFC0 rd, <nn>
		if (rs == 0)
		{
			mnemonic = "MFC0";
			disasm("%s r%d, $%d", mnemonic.c_str(), rt, rd);

			uint32_t tmp = cpu->COP0[rd];
			cpu->reg[rt] = tmp;
		}

		// Move to co-processor zero
		// MTC0 rs, <nn>
		else if (rs == 4)
		{
			mnemonic = "MTC0";
			disasm("%s r%d, $%d", mnemonic.c_str(), rt, rd);

			uint32_t tmp = cpu->reg[rt];
			cpu->COP0[rd] = tmp;
			if (rd == 12) IsC = (tmp & 0x10000) ? true : false;
		}
	}

	// R Type
	else if (op == b("000000")) {
		// Jump Register
		// JR rs
		if (fun == b("001000")) {
			mnemonic = "JR";
			disasm("%s r%d", mnemonic.c_str(), rs);
			cpu->shouldJump = true;
			isJumpCycle = false;
			cpu->jumpPC = cpu->reg[rs];
		}

		// Jump Register
		// JALR
		else if (fun == b("001001")) {
			mnemonic = "JALR";
			disasm("%s r%d r%d", mnemonic.c_str(), rd, rs);
			cpu->shouldJump = true;
			isJumpCycle = false;
			cpu->jumpPC = cpu->reg[rs];
			cpu->reg[rd] = cpu->PC + 4;
		}

		// Add
		// add rd, rs, rt
		else if (fun == b("100000")) {
			mnemonic = "ADD";
			disasm("%s r%d, r%d, r%d", mnemonic.c_str(), rd, rs, rt);
			cpu->reg[rd] = ((int32_t)cpu->reg[rs]) + ((int32_t)cpu->reg[rt]);
		}

		// Add unsigned
		// add rd, rs, rt
		else if (fun == b("100001")) {
			mnemonic = "ADD";
			disasm("%s r%d, r%d, r%d", mnemonic.c_str(), rd, rs, rt);
			cpu->reg[rd] = cpu->reg[rs] + cpu->reg[rt];
		}

		// And
		// and rd, rs, rt
		else if (fun == b("100100")) {
			mnemonic = "AND";
			disasm("%s r%d, r%d, r%d", mnemonic.c_str(), rd, rs, rt);

			cpu->reg[rd] = cpu->reg[rs] & cpu->reg[rt];
		}

		// TODO
		// Divide
		// div rs, rt
		else if (fun == b("011010")) {
			mnemonic = "DIV UNSUPPORTED";
			disasm("%s r%d, r%d", mnemonic.c_str(), rs, rt);
		}

		// TODO
		// Multiply
		// mul rs, rt
		else if (fun == b("011000")) {
			mnemonic = "MULT";
			disasm("%s r%d, r%d", mnemonic.c_str(), rs, rt);

			uint64_t temp = cpu->reg[rs] * cpu->reg[rt];

			cpu->lo = temp & 0xffffffff;
			cpu->hi = (temp & 0xffffffff00000000) >> 32;
		}


		// Nor
		// NOR rd, rs, rt
		else if (fun == b("100111")) {
			mnemonic = "OR";
			disasm("%s r%d, r%d, r%d", mnemonic.c_str(), rd, rs, rt);

			cpu->reg[rd] = ~(cpu->reg[rs] | cpu->reg[rt]);
		}

		// Or
		// OR rd, rs, rt
		else if (fun == b("100101")) {
			mnemonic = "OR";
			disasm("%s r%d, r%d, r%d", mnemonic.c_str(), rd, rs, rt);

			cpu->reg[rd] = cpu->reg[rs] | cpu->reg[rt];
		}

		// Shift Word Left Logical
		// SLL rd, rt, a
		else if (fun == b("000000")) {
			if (rt == 0 && rd == 0 && sh == 0) {
				mnemonic = "NOP";
				disasm("%s", mnemonic.c_str());
			}
			else {
				mnemonic = "SLL";
				disasm("%s r%d, r%d, %d", mnemonic.c_str(), rd, rt, sh);

				cpu->reg[rd] = cpu->reg[rt] << sh;
			}
		}

		// Shift Word Left Logical Variable
		// SLLV rd, rt, rs
		else if (fun == b("000100")) {
			mnemonic = "SLLV";
			disasm("%s r%d, r%d, r%d", mnemonic.c_str(), rd, rt, rs);

			cpu->reg[rd] = cpu->reg[rt] << cpu->reg[rs];
		}

		// Shift Word Right Logical
		// SRA rd, rt, a
		else if (fun == b("000011")) {
			mnemonic = "SRA";
			disasm("%s r%d, r%d, %d", mnemonic.c_str(), rd, rt, sh);

			cpu->reg[rd] = cpu->reg[rt] >> sh;
		}

		// Shift Word Right Logical Variable
		// SRAV rd, rt, rs
		else if (fun == b("000111")) {
			mnemonic = "SRAV";
			disasm("%s r%d, r%d, r%d", mnemonic.c_str(), rd, rt, rs);

			cpu->reg[rd] = cpu->reg[rt] >> cpu->reg[rs];
		}

		// Shift Word Right Arithmetic
		// SRA rd, rt, a
		else if (fun == b("000010")) {
			mnemonic = "SRA UNSUPPORTED";
			disasm("%s r%d, r%d, %d", mnemonic.c_str(), rd, rt, sh);

			cpu->reg[rd] = cpu->reg[rt] >> sh;
		}

		// Shift Word Right Arithmetic Variable
		// SRAV rd, rt, rs
		else if (fun == b("100010")) {
			mnemonic = "SRAV UNSUPPORTED";
			disasm("%s r%d, r%d, r%d", mnemonic.c_str(), rd, rt, rs);

			cpu->reg[rd] = cpu->reg[rt] >> cpu->reg[rs];
		}

		// Xor
		// XOR rd, rs, rt
		else if (fun == b("100110")) {
			mnemonic = "XOR";
			disasm("%s r%d, r%d, r%d", mnemonic.c_str(), rd, rs, rt);

			cpu->reg[rd] = cpu->reg[rs] ^ cpu->reg[rt];
		}

		// Subtract
		// sub rd, rs, rt
		else if (fun == b("100010")) {
			mnemonic = "SUB";
			disasm("%s r%d, r%d, r%d", mnemonic.c_str(), rd, rs, rt);
			cpu->reg[rd] = ((int32_t)cpu->reg[rs]) - ((int32_t)cpu->reg[rt]);
		}

		// Subtract unsigned
		// subu rd, rs, rt
		else if (fun == b("100011")) {
			mnemonic = "SUBU";
			disasm("%s r%d, r%d, r%d", mnemonic.c_str(), rd, rs, rt);
			cpu->reg[rd] = ((int32_t)cpu->reg[rs]) - ((int32_t)cpu->reg[rt]);
		}


		// Set On Less Than Unsigned
		// SLTU rd, rs, rt
		else if (fun == b("101011")) {
			mnemonic = "SLTU";
			disasm("%s r%d, r%d, r%d", mnemonic.c_str(), rd, rs, rt);

			if (cpu->reg[rs] < cpu->reg[rt]) cpu->reg[rd] = 1;
			else cpu->reg[rd] = 0;
		}
		
	}

	else
	{
		return false;
	}

#undef b

	if (mnemonic == "NOP")
		nopCounter++;
	else nopCounter = 0;

	if (nopCounter > 30) {
		disassemblyEnabled = true;
		nopCounter = 0; 
		__debugbreak();
		//putFileContents("ram.bin", ram);
	}

	if (disassemblyEnabled) {
		if (_disasm.empty()) printf("%s\n", mnemonic.c_str());
		else printf("    0x%08x  %08x:        %s\n", cpu->PC - 4, readMemory32(cpu->PC - 4), _disasm.c_str());
	}

	//if (cpu->PC - 4 == 0xbfc01a78) __debugbreak();
	//if (rs == 29)// rd == 29 || rt == 29)
	//{
	//	__debugbreak();
	//}

	if (cpu->shouldJump && isJumpCycle) {
		cpu->PC = cpu->jumpPC & 0xFFFFFFFC;
		cpu->jumpPC = 0;
		cpu->shouldJump = false;
	}
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
	io.resize(8*1024);

	bool doDump = false;

	while (cpuRunning)
	{
		uint32_t opcode = readMemory32(cpu.PC);

		if (cpu.PC == 0x00003b88)
		{
			__debugbreak();
		}
		cpu.PC += 4;

		bool executed = executeInstruction(&cpu, opcode);

		if (doDump)
		{
			putFileContents("ram.bin", ram);
			doDump = false;
		}
		if (!executed)
		{
			//cpuRunning = false;
			printf("Unknown instruction: 0x%08x (copy: %02x %02x %02x %02x)\n", opcode, opcode & 0xff, (opcode >> 8) & 0xff, (opcode >> 16) & 0xff, (opcode >> 24) & 0xff);
			fflush(stdout);
		}
	}

	return 0;
}