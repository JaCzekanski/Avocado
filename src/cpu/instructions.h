#pragma once
#include <cstdint>
#include "mips.h"
#include "opcode.h"

namespace instructions {
using namespace mips;
typedef void (*_Instruction)(CPU*, Opcode);

struct PrimaryInstruction {
    uint32_t number;
    _Instruction instruction;
};

void exception(CPU* cpu, COP0::CAUSE::Exception cause);

void dummy(CPU* cpu, Opcode i);
void invalid(CPU* cpu, Opcode i);
void notImplemented(CPU* cpu, Opcode i);
void special(CPU* cpu, Opcode i);
void branch(CPU* cpu, Opcode i);

void op_sll(CPU* cpu, Opcode i);
void op_srl(CPU* cpu, Opcode i);
void op_sra(CPU* cpu, Opcode i);
void op_sllv(CPU* cpu, Opcode i);
void op_srlv(CPU* cpu, Opcode i);
void op_srav(CPU* cpu, Opcode i);
void op_jr(CPU* cpu, Opcode i);
void op_jalr(CPU* cpu, Opcode i);
void op_syscall(CPU* cpu, Opcode i);
void op_break(CPU* cpu, Opcode i);
void op_mfhi(CPU* cpu, Opcode i);
void op_mthi(CPU* cpu, Opcode i);
void op_mflo(CPU* cpu, Opcode i);
void op_mtlo(CPU* cpu, Opcode i);
void op_mult(CPU* cpu, Opcode i);
void op_multu(CPU* cpu, Opcode i);
void op_div(CPU* cpu, Opcode i);
void op_divu(CPU* cpu, Opcode i);
void op_add(CPU* cpu, Opcode i);
void op_addu(CPU* cpu, Opcode i);
void op_sub(CPU* cpu, Opcode i);
void op_subu(CPU* cpu, Opcode i);
void op_and(CPU* cpu, Opcode i);
void op_or(CPU* cpu, Opcode i);
void op_xor(CPU* cpu, Opcode i);
void op_nor(CPU* cpu, Opcode i);
void op_slt(CPU* cpu, Opcode i);
void op_sltu(CPU* cpu, Opcode i);

void op_j(CPU* cpu, Opcode i);
void op_jal(CPU* cpu, Opcode i);
void op_beq(CPU* cpu, Opcode i);
void op_bgtz(CPU* cpu, Opcode i);
void op_blez(CPU* cpu, Opcode i);
void op_bne(CPU* cpu, Opcode i);
void op_addi(CPU* cpu, Opcode i);
void op_addiu(CPU* cpu, Opcode i);
void op_slti(CPU* cpu, Opcode i);
void op_sltiu(CPU* cpu, Opcode i);
void op_andi(CPU* cpu, Opcode i);
void op_ori(CPU* cpu, Opcode i);
void op_xori(CPU* cpu, Opcode i);
void op_lui(CPU* cpu, Opcode i);
void op_cop0(CPU* cpu, Opcode i);
void op_cop2(CPU* cpu, Opcode i);
void op_lb(CPU* cpu, Opcode i);
void op_lh(CPU* cpu, Opcode i);
void op_lwl(CPU* cpu, Opcode i);
void op_lw(CPU* cpu, Opcode i);
void op_lbu(CPU* cpu, Opcode i);
void op_lhu(CPU* cpu, Opcode i);
void op_lwr(CPU* cpu, Opcode i);
void op_sb(CPU* cpu, Opcode i);
void op_sh(CPU* cpu, Opcode i);
void op_sh(CPU* cpu, Opcode i);
void op_swl(CPU* cpu, Opcode i);
void op_sw(CPU* cpu, Opcode i);
void op_swr(CPU* cpu, Opcode i);
void op_lwc2(CPU* cpu, Opcode i);
void op_swc2(CPU* cpu, Opcode i);
void op_breakpoint(CPU* cpu, Opcode i);

extern PrimaryInstruction OpcodeTable[64];
}
