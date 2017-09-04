#pragma once
#include <cstdint>
#include "mips.h"
#include "opcode.h"

namespace mipsInstructions {
using namespace mips;
typedef void (*_Instruction)(mips::CPU *, Opcode);
struct PrimaryInstruction {
    uint32_t number;
    _Instruction instruction;
};

void exception(mips::CPU *cpu, mips::cop0::CAUSE::Exception cause);

void dummy(mips::CPU *cpu, Opcode i);
void invalid(mips::CPU *cpu, Opcode i);
void notImplemented(mips::CPU *cpu, Opcode i);
void special(mips::CPU *cpu, Opcode i);
void branch(mips::CPU *cpu, Opcode i);

void op_sll(mips::CPU *cpu, Opcode i);
void op_srl(mips::CPU *cpu, Opcode i);
void op_sra(mips::CPU *cpu, Opcode i);
void op_sllv(mips::CPU *cpu, Opcode i);
void op_srlv(mips::CPU *cpu, Opcode i);
void op_srav(mips::CPU *cpu, Opcode i);
void op_jr(mips::CPU *cpu, Opcode i);
void op_jalr(mips::CPU *cpu, Opcode i);
void op_syscall(mips::CPU *cpu, Opcode i);
void op_break(mips::CPU *cpu, Opcode i);
void op_mfhi(mips::CPU *cpu, Opcode i);
void op_mthi(mips::CPU *cpu, Opcode i);
void op_mflo(mips::CPU *cpu, Opcode i);
void op_mtlo(mips::CPU *cpu, Opcode i);
void op_mult(mips::CPU *cpu, Opcode i);
void op_multu(mips::CPU *cpu, Opcode i);
void op_div(mips::CPU *cpu, Opcode i);
void op_divu(mips::CPU *cpu, Opcode i);
void op_add(mips::CPU *cpu, Opcode i);
void op_addu(mips::CPU *cpu, Opcode i);
void op_sub(mips::CPU *cpu, Opcode i);
void op_subu(mips::CPU *cpu, Opcode i);
void op_and(mips::CPU *cpu, Opcode i);
void op_or(mips::CPU *cpu, Opcode i);
void op_xor(mips::CPU *cpu, Opcode i);
void op_nor(mips::CPU *cpu, Opcode i);
void op_slt(mips::CPU *cpu, Opcode i);
void op_sltu(mips::CPU *cpu, Opcode i);

void op_j(mips::CPU *cpu, Opcode i);
void op_jal(mips::CPU *cpu, Opcode i);
void op_beq(mips::CPU *cpu, Opcode i);
void op_bgtz(mips::CPU *cpu, Opcode i);
void op_blez(mips::CPU *cpu, Opcode i);
void op_bne(mips::CPU *cpu, Opcode i);
void op_addi(mips::CPU *cpu, Opcode i);
void op_addiu(mips::CPU *cpu, Opcode i);
void op_slti(mips::CPU *cpu, Opcode i);
void op_sltiu(mips::CPU *cpu, Opcode i);
void op_andi(mips::CPU *cpu, Opcode i);
void op_ori(mips::CPU *cpu, Opcode i);
void op_xori(mips::CPU *cpu, Opcode i);
void op_lui(mips::CPU *cpu, Opcode i);
void op_cop0(mips::CPU *cpu, Opcode i);
void op_cop2(mips::CPU *cpu, Opcode i);
void op_lb(mips::CPU *cpu, Opcode i);
void op_lh(mips::CPU *cpu, Opcode i);
void op_lwl(mips::CPU *cpu, Opcode i);
void op_lw(mips::CPU *cpu, Opcode i);
void op_lbu(mips::CPU *cpu, Opcode i);
void op_lhu(mips::CPU *cpu, Opcode i);
void op_lwr(mips::CPU *cpu, Opcode i);
void op_sb(mips::CPU *cpu, Opcode i);
void op_sh(mips::CPU *cpu, Opcode i);
void op_sh(mips::CPU *cpu, Opcode i);
void op_swl(mips::CPU *cpu, Opcode i);
void op_sw(mips::CPU *cpu, Opcode i);
void op_swr(mips::CPU *cpu, Opcode i);
void op_lwc2(mips::CPU *cpu, Opcode i);
void op_swc2(mips::CPU *cpu, Opcode i);
void op_breakpoint(mips::CPU *cpu, Opcode i);

extern PrimaryInstruction OpcodeTable[64];
};
