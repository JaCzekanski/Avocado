#include "mipsInstructions.h"
#include <string>
#include "utils/string.h"

PrimaryInstruction OpcodeTable[64] =
{
	{ 0, mipsInstructions::special, "special" }, // R type
	{ 1, mipsInstructions::branch, "branch" },
	{ 2, mipsInstructions::j, "j" },
	{ 3, mipsInstructions::jal, "jal" },
	{ 4, mipsInstructions::beq, "beq" },
	{ 5, mipsInstructions::bne, "bne" },
	{ 6, mipsInstructions::blez, "blez" },
	{ 7, mipsInstructions::bgtz, "bgtz" },

	{ 8, mipsInstructions::addi, "addi" },
	{ 9, mipsInstructions::addiu, "addiu" },
	{ 10, mipsInstructions::slti, "slti" },
	{ 11, mipsInstructions::sltiu, "sltiu" },
	{ 12, mipsInstructions::andi, "andi" },
	{ 13, mipsInstructions::ori, "ori" },
	{ 14, mipsInstructions::xori, "xori" },
	{ 15, mipsInstructions::lui, "lui" },

	{ 16, mipsInstructions::cop0, "cop0" },
	{ 17, mipsInstructions::notImplemented, "cop1" },
	{ 18, mipsInstructions::notImplemented, "cop2" },
	{ 19, mipsInstructions::notImplemented, "cop3" },
	{ 20, mipsInstructions::invalid, "INVALID" },
	{ 21, mipsInstructions::invalid, "INVALID" },
	{ 22, mipsInstructions::invalid, "INVALID" },
	{ 23, mipsInstructions::invalid, "INVALID" },

	{ 24, mipsInstructions::invalid, "INVALID" },
	{ 25, mipsInstructions::invalid, "INVALID" },
	{ 26, mipsInstructions::invalid, "INVALID" },
	{ 27, mipsInstructions::invalid, "INVALID" },
	{ 28, mipsInstructions::invalid, "INVALID" },
	{ 29, mipsInstructions::invalid, "INVALID" },
	{ 30, mipsInstructions::invalid, "INVALID" },
	{ 31, mipsInstructions::invalid, "INVALID" },

	{ 32, mipsInstructions::lb, "lb" },
	{ 33, mipsInstructions::lh, "lh" },
	{ 34, mipsInstructions::notImplemented, "lwl" },
	{ 35, mipsInstructions::lw, "lw" },
	{ 36, mipsInstructions::lbu, "lbu" },
	{ 37, mipsInstructions::lbu, "lhu" },
	{ 38, mipsInstructions::notImplemented, "lwr" },
	{ 39, mipsInstructions::invalid, "INVALID" },

	{ 40, mipsInstructions::sb, "sb" },
	{ 41, mipsInstructions::sh, "sh" },
	{ 42, mipsInstructions::notImplemented, "swl" },
	{ 43, mipsInstructions::sw, "sw" },
	{ 44, mipsInstructions::invalid, "INVALID" },
	{ 45, mipsInstructions::invalid, "INVALID" },
	{ 46, mipsInstructions::notImplemented, "swr" },
	{ 47, mipsInstructions::invalid, "INVALID" },

	{ 48, mipsInstructions::notImplemented, "lwc0" },
	{ 49, mipsInstructions::notImplemented, "lwc1" },
	{ 50, mipsInstructions::notImplemented, "lwc2" },
	{ 51, mipsInstructions::notImplemented, "lwc3" },
	{ 52, mipsInstructions::invalid, "INVALID" },
	{ 53, mipsInstructions::invalid, "INVALID" },
	{ 54, mipsInstructions::invalid, "INVALID" },
	{ 55, mipsInstructions::invalid, "INVALID" },

	{ 56, mipsInstructions::notImplemented, "swc0" },
	{ 57, mipsInstructions::notImplemented, "swc1" },
	{ 58, mipsInstructions::notImplemented, "swc2" },
	{ 59, mipsInstructions::notImplemented, "swc3" },
	{ 60, mipsInstructions::invalid, "INVALID" },
	{ 61, mipsInstructions::invalid, "INVALID" },
	{ 62, mipsInstructions::invalid, "INVALID" },
	{ 63, mipsInstructions::invalid, "INVALID" },
};

PrimaryInstruction SpecialTable[64] =
{
	{ 0, mipsInstructions::sll, "sll" },
	{ 1, mipsInstructions::invalid, "INVALID" },
	{ 2, mipsInstructions::srl, "stl" },
	{ 3, mipsInstructions::sra, "sra" },
	{ 4, mipsInstructions::sllv, "sllv" },
	{ 5, mipsInstructions::invalid, "INVALID" },
	{ 6, mipsInstructions::srlv, "srlv" },
	{ 7, mipsInstructions::srav, "srav" },

	{ 8, mipsInstructions::jr, "jr" },
	{ 9, mipsInstructions::jalr, "jalr" },
	{ 10, mipsInstructions::invalid, "INVALID" },
	{ 11, mipsInstructions::invalid, "INVALID" },
	{ 12, mipsInstructions::syscall, "syscall" },
	{ 13, mipsInstructions::notImplemented, "break" },
	{ 14, mipsInstructions::invalid, "INVALID" },
	{ 15, mipsInstructions::invalid, "INVALID" },

	{ 16, mipsInstructions::mfhi, "mfhi" },
	{ 17, mipsInstructions::mthi, "mthi" },
	{ 18, mipsInstructions::mflo, "mflo" },
	{ 19, mipsInstructions::mtlo, "mtlo" },
	{ 20, mipsInstructions::invalid, "INVALID" },
	{ 21, mipsInstructions::invalid, "INVALID" },
	{ 22, mipsInstructions::invalid, "INVALID" },
	{ 23, mipsInstructions::invalid, "INVALID" },

	{ 24, mipsInstructions::mult, "mult" },
	{ 25, mipsInstructions::multu, "multu" },
	{ 26, mipsInstructions::div, "div" },
	{ 27, mipsInstructions::divu, "divu" },
	{ 28, mipsInstructions::invalid, "INVALID" },
	{ 29, mipsInstructions::invalid, "INVALID" },
	{ 30, mipsInstructions::invalid, "INVALID" },
	{ 31, mipsInstructions::invalid, "INVALID" },

	{ 32, mipsInstructions::add, "add" },
	{ 33, mipsInstructions::addu, "addu" },
	{ 34, mipsInstructions::sub, "sub" },
	{ 35, mipsInstructions::subu, "subu" },
	{ 36, mipsInstructions::and, "and" },
	{ 37, mipsInstructions::or, "or" },
	{ 38, mipsInstructions::xor, "xor" },
	{ 39, mipsInstructions::nor, "nor" },

	{ 40, mipsInstructions::invalid, "INVALID" },
	{ 41, mipsInstructions::invalid, "INVALID" },
	{ 42, mipsInstructions::slt, "slt" },
	{ 43, mipsInstructions::sltu, "sltu" },
	{ 44, mipsInstructions::invalid, "INVALID" },
	{ 45, mipsInstructions::invalid, "INVALID" },
	{ 46, mipsInstructions::invalid, "INVALID" },
	{ 47, mipsInstructions::invalid, "INVALID" },

	{ 48, mipsInstructions::invalid, "INVALID" },
	{ 49, mipsInstructions::invalid, "INVALID" },
	{ 50, mipsInstructions::invalid, "INVALID" },
	{ 51, mipsInstructions::invalid, "INVALID" },
	{ 52, mipsInstructions::invalid, "INVALID" },
	{ 53, mipsInstructions::invalid, "INVALID" },
	{ 54, mipsInstructions::invalid, "INVALID" },
	{ 55, mipsInstructions::invalid, "INVALID" },

	{ 56, mipsInstructions::invalid, "INVALID" },
	{ 57, mipsInstructions::invalid, "INVALID" },
	{ 58, mipsInstructions::invalid, "INVALID" },
	{ 59, mipsInstructions::invalid, "INVALID" },
	{ 60, mipsInstructions::invalid, "INVALID" },
	{ 61, mipsInstructions::invalid, "INVALID" },
	{ 62, mipsInstructions::invalid, "INVALID" },
	{ 63, mipsInstructions::invalid, "INVALID" },
};

extern bool disassemblyEnabled;
extern bool IsC;
extern char* _mnemonic;
extern std::string _disasm;

uint8_t readMemory8(uint32_t address);
uint16_t readMemory16(uint32_t address);
uint32_t readMemory32(uint32_t address);
void writeMemory8(uint32_t address, uint8_t data);
void writeMemory16(uint32_t address, uint16_t data);
void writeMemory32(uint32_t address, uint32_t data);

namespace mipsInstructions
{
	void invalid(CPU* cpu, Opcode i)
	{
		printf("Invalid opcode (%s) at 0x%08x: 0x%08x\n", OpcodeTable[i.op].mnemnic, cpu->PC, i.opcode);
		cpu->halted = true;
	}

	void notImplemented(CPU* cpu, Opcode i)
	{
		printf("Opcode %s not implemented at 0x%08x: 0x%08x\n", OpcodeTable[i.op].mnemnic, cpu->PC, i.opcode);
		cpu->halted = true;
	}

	void special(CPU* cpu, Opcode i)
	{
		const auto &instruction = SpecialTable[i.fun];
		mnemonic(instruction.mnemnic);
		instruction.instruction(cpu, i);
		// TODO: break
	}

	// Shift Word Left Logical
	// SLL rd, rt, a
	void sll(CPU *cpu, Opcode i)
	{
		if (i.rt == 0 && i.rd == 0 && i.sh == 0) 
		{
			mnemonic("nop");
			disasm(" ");
		}
		else 
		{
			disasm("r%d, r%d, %d", i.rd, i.rt, i.sh);
			cpu->reg[i.rd] = cpu->reg[i.rt] << i.sh;
		}
	}

	// Shift Word Right Logical
	// SRL rd, rt, a
	void srl(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, %d", i.rd, i.rt, i.sh);
		cpu->reg[i.rd] = cpu->reg[i.rt] >> i.sh;
	}

	// Shift Word Right Arithmetic
	// SRA rd, rt, a
	void sra(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, %d", i.rd, i.rt, i.sh);
		cpu->reg[i.rd] = ((int32_t)cpu->reg[i.rt]) >> i.sh;
	}

	// Shift Word Left Logical Variable
	// SLLV rd, rt, rs
	void sllv(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, r%d", i.rd, i.rt, i.rs);
		cpu->reg[i.rd] = cpu->reg[i.rt] << (cpu->reg[i.rs] & 0x1f);
	}

	// Shift Word Right Logical Variable
	// SRLV rd, rt, a
	void srlv(CPU *cpu, Opcode i) 
	{
		disasm("r%d, r%d, %d", i.rd, i.rt, i.sh);
		cpu->reg[i.rd] = cpu->reg[i.rt] >> (cpu->reg[i.rs] & 0x1f);
	}

	// Shift Word Right Arithmetic Variable
	// SRAV rd, rt, rs
	void srav(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, r%d", i.rd, i.rt, i.rs);
		cpu->reg[i.rd] = ((int32_t)cpu->reg[i.rt]) >> (cpu->reg[i.rs] & 0x1f);
	}

	// Jump Register
	// JR rs
	void jr(CPU *cpu, Opcode i)
	{
		disasm("r%d", i.rs);
		cpu->shouldJump = true;
		cpu->jumpPC = cpu->reg[i.rs];
	}

	// Jump Register
	// JALR
	void jalr(CPU *cpu, Opcode i)
	{
		disasm("r%d r%d", i.rd, i.rs);
		cpu->shouldJump = true;
		cpu->jumpPC = cpu->reg[i.rs];
		cpu->reg[i.rd] = cpu->PC + 8;
	}

	// Syscall
	// SYSCALL
	void syscall(CPU *cpu, Opcode i)
	{
		cpu->COP0[14] = cpu->PC+4; // EPC - return address from trap
		cpu->COP0[13] = 8 << 2; // Cause, hardcoded SYSCALL
		cpu->PC = 0x80000080-4;
	}

	// Move From Hi
	// MFHI rd
	void mfhi(CPU *cpu, Opcode i)
	{
		disasm("r%d", i.rd);
		cpu->reg[i.rd] = cpu->hi;
	}

	// Move To Hi
	// MTHI rd
	void mthi(CPU *cpu, Opcode i)
	{
		disasm("r%d", i.rd);
		cpu->hi = cpu->reg[i.rd];
	}

	// Move From Lo
	// MFLO rd
	void mflo(CPU *cpu, Opcode i)
	{
		disasm("r%d", i.rd);
		cpu->reg[i.rd] = cpu->lo;
	}

	// Move To Lo
	// MTLO rd
	void mtlo(CPU *cpu, Opcode i)
	{
		disasm("r%d", i.rd);
		cpu->lo = cpu->reg[i.rd];
	}

	// Multiply
	// mult rs, rt
	void mult(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d", i.rs, i.rt);
		int64_t temp = (int64_t)cpu->reg[i.rs] * (int64_t)cpu->reg[i.rt];
		cpu->lo = temp & 0xffffffff;
		cpu->hi = (temp & 0xffffffff00000000) >> 32;
	}

	// Multiply Unsigned
	// multu rs, rt
	void multu(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d", i.rs, i.rt);
		int64_t temp = (uint64_t)cpu->reg[i.rs] * (uint64_t)cpu->reg[i.rt];
		cpu->lo = temp & 0xffffffff;
		cpu->hi = (temp & 0xffffffff00000000) >> 32;
	}
	// Divide
	// div rs, rt
	void div(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d", i.rs, i.rt);
		// TODO: Check for 0
		cpu->lo = (int32_t)cpu->reg[i.rs] / (int32_t)cpu->reg[i.rt];
		cpu->hi = (int32_t)cpu->reg[i.rs] % (int32_t)cpu->reg[i.rt];
	}

	// Divide Unsigned Word
	// divu rs, rt
	void divu(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d", i.rs, i.rt);
		// TODO: Check for 0
		cpu->lo = cpu->reg[i.rs] / cpu->reg[i.rt];
		cpu->hi = cpu->reg[i.rs] % cpu->reg[i.rt];
	}

	// add rd, rs, rt
	void add(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
		cpu->reg[i.rd] = cpu->reg[i.rs] + cpu->reg[i.rt];
	}

	// Add unsigned
	// addu rd, rs, rt
	void addu(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
		cpu->reg[i.rd] = cpu->reg[i.rs] + cpu->reg[i.rt];
	}

	// Subtract
	// sub rd, rs, rt
	void sub(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
		cpu->reg[i.rd] = cpu->reg[i.rs] - cpu->reg[i.rt];
	}

	// Subtract unsigned
	// subu rd, rs, rt
	void subu(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
		cpu->reg[i.rd] = cpu->reg[i.rs] - cpu->reg[i.rt];
	}

	// And
	// and rd, rs, rt
	void and(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
		cpu->reg[i.rd] = cpu->reg[i.rs] & cpu->reg[i.rt];
	}

	// Or
	// OR rd, rs, rt
	void or(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
		cpu->reg[i.rd] = cpu->reg[i.rs] | cpu->reg[i.rt];
	}

	// Xor
	// XOR rd, rs, rt
	void xor(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
		cpu->reg[i.rd] = cpu->reg[i.rs] ^ cpu->reg[i.rt];
	}

	// Nor
	// NOR rd, rs, rt
	void nor(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
		cpu->reg[i.rd] = ~(cpu->reg[i.rs] | cpu->reg[i.rt]);
	}

	// Set On Less Than Signed
	// SLT rd, rs, rt
	void slt(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
		if ((int32_t)cpu->reg[i.rs] < (int32_t)cpu->reg[i.rt]) cpu->reg[i.rd] = 1;
		else cpu->reg[i.rd] = 0;
	}

	// Set On Less Than Unsigned
	// SLTU rd, rs, rt
	void sltu(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
		if (cpu->reg[i.rs] < cpu->reg[i.rt]) cpu->reg[i.rd] = 1;
		else cpu->reg[i.rd] = 0;
	}

	void branch(CPU* cpu, Opcode i)
	{
		// Branch On Less Than Zero
		// BLTZ rs, offset
		if (i.rt == 0) {
			mnemonic("BLTZ");
			disasm("r%d, %d", i.rs, i.offset);

			if ((int32_t)cpu->reg[i.rs] < 0) {
				cpu->shouldJump = true;
				cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset << 2);
			}
		}

		// Branch On Greater Than Or Equal To Zero
		// BGEZ rs, offset
		else if (i.rt == 1) {
		 	mnemonic("BGEZ");
			disasm("r%d, %d", i.rs, i.offset);

		 	if ((int32_t)cpu->reg[i.rs] >= 0) {
		 		cpu->shouldJump = true;
				cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset << 2);
		 	}
		}

		// Branch On Less Than Zero And Link
		// bltzal rs, offset
		else if (i.rt == 16) {
			mnemonic("BLTZAL");
			disasm("r%d, %d", i.rs, i.offset);

			cpu->reg[31] = cpu->PC + 8;
			if ((int32_t)cpu->reg[i.rs] < 0) {
				cpu->shouldJump = true;
				cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset << 2);
			}
		}

		// Branch On Greater Than Or Equal To Zero And Link
		// BGEZAL rs, offset
		else if (i.rt == 17) {
		 	mnemonic("BGEZAL");
			disasm("r%d, %d", i.rs, i.offset);

		 	cpu->reg[31] = cpu->PC + 8;
		 	if ((int32_t)cpu->reg[i.rs] >= 0) {
		 		cpu->shouldJump = true;
				cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset << 2);
		 	}
		}
		else invalid(cpu, i);
	}

	// Jump
	// J target
	void j(CPU *cpu, Opcode i)
	{
		disasm("0x%x", i.target << 2);
		cpu->shouldJump = true;
		cpu->jumpPC = (cpu->PC & 0xf0000000) | (i.target << 2);
	}

	// Jump And Link
	// JAL target
	void jal(CPU *cpu, Opcode i)
	{
		disasm("0x%x", (i.target << 2));
		cpu->shouldJump = true;
		cpu->jumpPC = (cpu->PC & 0xf0000000) | (i.target << 2);
		cpu->reg[31] = cpu->PC + 8;
	}

	// Branch On Equal
	// BEQ rs, rt, offset
	void beq(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, %d", i.rs, i.rt, i.offset);
		if (cpu->reg[i.rt] == cpu->reg[i.rs]) {
			cpu->shouldJump = true;
			cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset << 2);
		}
	}

	// Branch On Greater Than Zero
	// BGTZ rs, offset
	void bgtz(CPU *cpu, Opcode i)
	{
		disasm("r%d, %d", i.rs, i.offset);
		if ((int32_t)cpu->reg[i.rs] > 0) {
			cpu->shouldJump = true;
			cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset << 2);
		}
	}

	// Branch On Less Than Or Equal To Zero
	// BLEZ rs, offset
	void blez(CPU *cpu, Opcode i)
	{
		disasm("r%d, %d", i.rs, i.offset);
		if ((int32_t)cpu->reg[i.rs] <= 0) {
			cpu->shouldJump = true;
			cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset << 2);
		}
	}

	// Branch On Not Equal
	// BNE rs, offset
	void bne(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, %d", i.rs, i.rt, i.offset);
		if (cpu->reg[i.rt] != cpu->reg[i.rs]) {
			cpu->shouldJump = true;
			cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset << 2);
		}
	}

	// Add Immediate Word
	// ADDI rt, rs, imm
	void addi(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, %d", i.rt, i.rs, i.offset);
		cpu->reg[i.rt] = (int32_t)cpu->reg[i.rs] + i.offset;
	}

	// Add Immediate Unsigned Word
	// ADDIU rt, rs, imm
	void addiu(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, %d", i.rt, i.rs, i.offset);
		cpu->reg[i.rt] = cpu->reg[i.rs] + i.offset;
	}

	// Set On Less Than Immediate 
	// SLTI rd, rs, rt
	void slti(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, 0x%x", i.rt, i.rs, i.imm);
		if ((int32_t)cpu->reg[i.rs] < (int32_t)i.offset) cpu->reg[i.rt] = 1;
		else cpu->reg[i.rt] = 0;
	}

	// Set On Less Than Immediate Unsigned
	// SLTIU rd, rs, rt
	void sltiu(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, 0x%x", i.rt, i.rs, i.imm);
		if (cpu->reg[i.rs] < (uint32_t)i.imm) cpu->reg[i.rt] = 1;
		else cpu->reg[i.rt] = 0;
	}

	// And Immediate
	// ANDI rt, rs, imm
	void andi(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, 0x%x", i.rt, i.rs, i.imm);
		cpu->reg[i.rt] = cpu->reg[i.rs] & i.imm;
	}

	// Or Immediete
	// ORI rt, rs, imm
	void ori(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, 0x%x", i.rt, i.rs, i.imm);
		cpu->reg[i.rt] = cpu->reg[i.rs] | i.imm;
	}

	// Xor Immediate
	// XORI rt, rs, imm
	void xori(CPU *cpu, Opcode i)
	{
		disasm("r%d, r%d, 0x%x", i.rt, i.rs, i.imm);
		cpu->reg[i.rt] = cpu->reg[i.rs] ^ i.imm;
	}

	// Load Upper Immediate
	// LUI rt, imm
	void lui(CPU *cpu, Opcode i)
	{
		disasm("r%d, 0x%x", i.rt, i.imm);
		cpu->reg[i.rt] = i.imm << 16;
	}

	// Coprocessor zero
	void cop0(CPU *cpu, Opcode i)
	{
		// Move from co-processor zero
		// MFC0 rd, <nn>
		if (i.rs == 0)
		{
			mnemonic("MFC0");
			disasm("r%d, $%d", i.rt, i.rd);

			uint32_t tmp = cpu->COP0[i.rd];
			cpu->reg[i.rt] = tmp;
		}

		// Move to co-processor zero
		// MTC0 rs, <nn>
		else if (i.rs == 4)
		{
			mnemonic("MTC0");
			disasm("r%d, $%d", i.rt, i.rd);

			uint32_t tmp = cpu->reg[i.rt];
			cpu->COP0[i.rd] = tmp;
			if (i.rd == 12) IsC = (tmp & 0x10000) ? true : false;
		}

		// Restore from exception
		// RFE
		else if (i.rs == 16)
		{
			if (i.fun == 16) 
			{
				mnemonic("RFE");
				printf("RFE TODO\n");
			}
			else invalid(cpu, i);
		}
		else invalid(cpu, i);
	}

	// Load Byte
	// LB rt, offset(base)
	void lb(CPU *cpu, Opcode i)
	{
		disasm("r%d, %d(r%d)", i.rt, i.offset, i.rs);
		uint32_t addr = cpu->reg[i.rs] + i.offset;
		cpu->reg[i.rt] = ((int32_t)(readMemory8(addr) << 24)) >> 24;
	}

	// Load Halfword 
	// LH rt, offset(base)
	void lh(CPU *cpu, Opcode i)
	{
		disasm("r%d, %d(r%d)", i.rt, i.offset, i.rs);
		uint32_t addr = cpu->reg[i.rs] + i.offset;
		cpu->reg[i.rt] = ((int32_t)(readMemory16(addr) << 16)) >> 16;
	}

	// Load Word
	// LW rt, offset(base)
	void lw(CPU *cpu, Opcode i)
	{
		disasm("r%d, %d(r%d)", i.rt, i.offset, i.rs);
		uint32_t addr = cpu->reg[i.rs] + i.offset;
		cpu->reg[i.rt] = readMemory32(addr);
	}

	// Load Byte Unsigned
	// LBU rt, offset(base)
	void lbu(CPU *cpu, Opcode i)
	{
		disasm("r%d, %d(r%d)", i.rt, i.offset, i.rs);
		uint32_t addr = cpu->reg[i.rs] + i.offset;
		cpu->reg[i.rt] = readMemory8(addr);
	}

	// Load Halfword Unsigned
	// LHU rt, offset(base)
	void lhu(CPU *cpu, Opcode i)
	{
		disasm("r%d, %d(r%d)", i.rt, i.offset, i.rs);
		uint32_t addr = cpu->reg[i.rs] + i.offset;
		cpu->reg[i.rt] = readMemory16(addr);
	}

	// Store Byte
	// SB rt, offset(base)
	void sb(CPU *cpu, Opcode i)
	{
		disasm("r%d, %d(r%d)", i.rt, i.offset, i.rs);
		uint32_t addr = cpu->reg[i.rs] + i.offset;
		writeMemory8(addr, cpu->reg[i.rt]);
	}

	// Store Halfword
	// SH rt, offset(base)
	void sh(CPU *cpu, Opcode i)
	{
		disasm("r%d, %d(r%d)", i.rt, i.offset, i.rs);
		uint32_t addr = cpu->reg[i.rs] + i.offset;
		writeMemory16(addr, cpu->reg[i.rt]);
	}

	// Store Word
	// SW rt, offset(base)
	void sw(CPU *cpu, Opcode i)
	{
		disasm("r%d, %d(r%d)", i.rt, i.offset, i.rs);
		uint32_t addr = cpu->reg[i.rs] + i.offset;
		writeMemory32(addr, cpu->reg[i.rt]);
	}
};