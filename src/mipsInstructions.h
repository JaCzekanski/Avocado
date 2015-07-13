#pragma once
#include <stdint.h>

#define mnemonic(x) if (disassemblyEnabled) _mnemonic = x
#define disasm(fmt, ...) if (disassemblyEnabled) _disasm=string_format(fmt, ##__VA_ARGS__)
#define pseudo(fmt, ...) if (disassemblyEnabled) _pseudo=string_format(fmt, ##__VA_ARGS__)

union Opcode
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

	struct
	{
		uint32_t fun : 6;
		uint32_t sh : 5;
		uint32_t rd : 5;
		uint32_t rt : 5;
		uint32_t rs : 5;
		uint32_t op : 6;
	};
	uint32_t opcode; // Whole 32bit opcode
	uint32_t target : 26; // JType instruction jump address
	uint16_t imm;    // IType immediate 
	int16_t offset;  // IType signed immediate (relative address)
};

struct CPU
{
	uint32_t PC;
	uint32_t jumpPC;
	bool shouldJump;
	bool isJumpCycle;
	uint32_t reg[32];
	uint32_t COP0[32];
	uint32_t hi, lo;
	bool IsC = false;
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
};

typedef void(*_Instruction) (CPU*, Opcode);
struct PrimaryInstruction
{
	uint8_t number;
	_Instruction instruction;
	char* mnemnic;
};

namespace mipsInstructions
{
	void invalid(CPU* cpu, Opcode i);
	void notImplemented(CPU* cpu, Opcode i);
	void special(CPU* cpu, Opcode i);
	void branch(CPU* cpu, Opcode i);
	void j(CPU *cpu, Opcode i);
	void jal(CPU *cpu, Opcode i);
	void beq(CPU *cpu, Opcode i);
	void bgtz(CPU *cpu, Opcode i);
	void blez(CPU *cpu, Opcode i);
	void bne(CPU *cpu, Opcode i);
	void addi(CPU *cpu, Opcode i);
	void addiu(CPU *cpu, Opcode i);
	void slti(CPU *cpu, Opcode i);
	void sltiu(CPU *cpu, Opcode i);
	void andi(CPU *cpu, Opcode i);
	void ori(CPU *cpu, Opcode i);
	void xori(CPU *cpu, Opcode i);
	void lui(CPU *cpu, Opcode i);
	void cop0(CPU *cpu, Opcode i);
	void lb(CPU *cpu, Opcode i);
	void lh(CPU *cpu, Opcode i);
	void lwu(CPU *cpu, Opcode i);
	void lw(CPU *cpu, Opcode i);
	void lbu(CPU *cpu, Opcode i);
	void lhu(CPU *cpu, Opcode i);
	void sb(CPU *cpu, Opcode i);
	void sh(CPU *cpu, Opcode i);
	void sh(CPU *cpu, Opcode i);
	void sw(CPU *cpu, Opcode i);
};

extern PrimaryInstruction OpcodeTable[64];