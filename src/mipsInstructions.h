#pragma once
#include <stdint.h>
#include "mips.h"

namespace mipsInstructions
{
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

	typedef void(*_Instruction) (mips::CPU*, Opcode);
	struct PrimaryInstruction
	{
		uint32_t number;
		_Instruction instruction;
		char* mnemnic;
	};

	void invalid(mips::CPU* cpu, Opcode i);
	void notImplemented(mips::CPU* cpu, Opcode i);
	void special(mips::CPU* cpu, Opcode i);

	void sll(mips::CPU* cpu, Opcode i);
	void srl(mips::CPU* cpu, Opcode i);
	void sra(mips::CPU* cpu, Opcode i);
	void sllv(mips::CPU* cpu, Opcode i);
	void srlv(mips::CPU* cpu, Opcode i);
	void srav(mips::CPU* cpu, Opcode i);
	void jr(mips::CPU* cpu, Opcode i);
	void jalr(mips::CPU* cpu, Opcode i);
	void syscall(mips::CPU* cpu, Opcode i);
	void break_(mips::CPU *cpu, Opcode i);
	void mfhi(mips::CPU* cpu, Opcode i);
	void mthi(mips::CPU* cpu, Opcode i);
	void mflo(mips::CPU* cpu, Opcode i);
	void mtlo(mips::CPU* cpu, Opcode i);
	void mult(mips::CPU* cpu, Opcode i);
	void multu(mips::CPU* cpu, Opcode i);
	void div(mips::CPU* cpu, Opcode i);
	void divu(mips::CPU* cpu, Opcode i);
	void add(mips::CPU* cpu, Opcode i);
	void addu(mips::CPU* cpu, Opcode i);
	void sub(mips::CPU* cpu, Opcode i);
	void subu(mips::CPU* cpu, Opcode i);
	void and(mips::CPU* cpu, Opcode i);
	void or(mips::CPU* cpu, Opcode i);
	void xor(mips::CPU* cpu, Opcode i);
	void nor(mips::CPU* cpu, Opcode i);
	void slt(mips::CPU* cpu, Opcode i);
	void sltu(mips::CPU* cpu, Opcode i);

	void branch(mips::CPU* cpu, Opcode i);
	void j(mips::CPU* cpu, Opcode i);
	void jal(mips::CPU* cpu, Opcode i);
	void beq(mips::CPU* cpu, Opcode i);
	void bgtz(mips::CPU* cpu, Opcode i);
	void blez(mips::CPU* cpu, Opcode i);
	void bne(mips::CPU* cpu, Opcode i);
	void addi(mips::CPU* cpu, Opcode i);
	void addiu(mips::CPU* cpu, Opcode i);
	void slti(mips::CPU* cpu, Opcode i);
	void sltiu(mips::CPU* cpu, Opcode i);
	void andi(mips::CPU* cpu, Opcode i);
	void ori(mips::CPU* cpu, Opcode i);
	void xori(mips::CPU* cpu, Opcode i);
	void lui(mips::CPU* cpu, Opcode i);
	void cop0(mips::CPU* cpu, Opcode i);
	void cop2(mips::CPU* cpu, Opcode i);
	void lb(mips::CPU* cpu, Opcode i);
	void lh(mips::CPU* cpu, Opcode i);
	void lwl(mips::CPU* cpu, Opcode i);
	void lwu(mips::CPU* cpu, Opcode i);
	void lw(mips::CPU* cpu, Opcode i);
	void lbu(mips::CPU* cpu, Opcode i);
	void lhu(mips::CPU* cpu, Opcode i);
	void lwr(mips::CPU* cpu, Opcode i);
	void sb(mips::CPU* cpu, Opcode i);
	void sh(mips::CPU* cpu, Opcode i);
	void sh(mips::CPU* cpu, Opcode i);
	void swl(mips::CPU* cpu, Opcode i);
	void sw(mips::CPU* cpu, Opcode i);
	void swr(mips::CPU* cpu, Opcode i);

	extern PrimaryInstruction OpcodeTable[64];
};
