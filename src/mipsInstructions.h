#pragma once
#include <stdint.h>
#include "mips.h"

namespace mipsInstructions {
union Opcode {
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

    // Example:
    // addu  rd,rs,rt
    // rd = rs+rt

    struct {
        uint32_t fun : 6;
        uint32_t sh : 5;
        uint32_t rd : 5;
        uint32_t rt : 5;
        uint32_t rs : 5;
        uint32_t op : 6;
    };
    uint32_t opcode;       // Whole 32bit opcode
    uint32_t target : 26;  // JType instruction jump address
    uint16_t imm;          // IType immediate
    int16_t offset;        // IType signed immediate (relative address)
};

typedef void (*_Instruction)(mips::CPU *, Opcode);
struct PrimaryInstruction {
    uint32_t number;
    _Instruction instruction;
    const char *mnemnic;
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
