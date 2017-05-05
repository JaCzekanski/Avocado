#include "mipsInstructions.h"
#include "mips.h"
#include "utils/string.h"

static const char *regNames[] = {"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
                                 "s0",   "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"};

#define mnemonic(x) \
    if (cpu->disassemblyEnabled) cpu->_mnemonic = (char *)(x)
#define disasm(fmt, ...) \
    if (cpu->disassemblyEnabled) cpu->_disasm = string_format(fmt, ##__VA_ARGS__)

using namespace mips;

namespace mipsInstructions {
PrimaryInstruction OpcodeTable[64] = {
    // R type
    {0, special, "special"},       //
    {1, branch, "branch"},         //
    {2, op_j, "j"},                //
    {3, op_jal, "jal"},            //
    {4, op_beq, "beq"},            //
    {5, op_bne, "bne"},            //
    {6, op_blez, "blez"},          //
    {7, op_bgtz, "bgtz"},          //
                                   //
    {8, op_addi, "addi"},          //
    {9, op_addiu, "addiu"},        //
    {10, op_slti, "slti"},         //
    {11, op_sltiu, "sltiu"},       //
    {12, op_andi, "andi"},         //
    {13, op_ori, "ori"},           //
    {14, op_xori, "xori"},         //
    {15, op_lui, "lui"},           //
                                   //
    {16, op_cop0, "cop0"},         //
    {17, notImplemented, "cop1"},  //
    {18, op_cop2, "cop2"},         //
    {19, notImplemented, "cop3"},  //
    {20, invalid, "INVALID"},      //
    {21, invalid, "INVALID"},      //
    {22, invalid, "INVALID"},      //
    {23, invalid, "INVALID"},      //
                                   //
    {24, invalid, "INVALID"},      //
    {25, invalid, "INVALID"},      //
    {26, invalid, "INVALID"},      //
    {27, invalid, "INVALID"},      //
    {28, invalid, "INVALID"},      //
    {29, invalid, "INVALID"},      //
    {30, invalid, "INVALID"},      //
    {31, invalid, "INVALID"},      //
                                   //
    {32, op_lb, "lb"},             //
    {33, op_lh, "lh"},             //
    {34, op_lwl, "lwl"},           //
    {35, op_lw, "lw"},             //
    {36, op_lbu, "lbu"},           //
    {37, op_lhu, "lhu"},           //
    {38, op_lwr, "lwr"},           //
    {39, invalid, "INVALID"},      //
                                   //
    {40, op_sb, "sb"},             //
    {41, op_sh, "sh"},             //
    {42, op_swl, "swl"},           //
    {43, op_sw, "sw"},             //
    {44, invalid, "INVALID"},      //
    {45, invalid, "INVALID"},      //
    {46, op_swr, "swr"},           //
    {47, invalid, "INVALID"},      //
                                   //
    {48, notImplemented, "lwc0"},  //
    {49, notImplemented, "lwc1"},  //
    {50, op_lwc2, "lwc2"},         //
    {51, notImplemented, "lwc3"},  //
    {52, invalid, "INVALID"},      //
    {53, invalid, "INVALID"},      //
    {54, invalid, "INVALID"},      //
    {55, invalid, "INVALID"},      //
                                   //
    {56, notImplemented, "swc0"},  //
    {57, notImplemented, "swc1"},  //
    {58, op_swc2, "swc2"},         //
    {59, notImplemented, "swc3"},  //
    {60, invalid, "INVALID"},      //
    {61, invalid, "INVALID"},      //
    {62, invalid, "INVALID"},      //
#ifdef ENABLE_BREAKPOINTS
    {63, op_breakpoint, "BREAKPOINT"}
#else
    {63, invalid, "INVALID"}
#endif
};

PrimaryInstruction SpecialTable[64] = {
    // opcodes encoded with "function" field, when opcode == 0
    {0, op_sll, "sll"},           //
    {1, invalid, "INVALID"},      //
    {2, op_srl, "srl"},           //
    {3, op_sra, "sra"},           //
    {4, op_sllv, "sllv"},         //
    {5, invalid, "INVALID"},      //
    {6, op_srlv, "srlv"},         //
    {7, op_srav, "srav"},         //
                                  //
    {8, op_jr, "jr"},             //
    {9, op_jalr, "jalr"},         //
    {10, invalid, "INVALID"},     //
    {11, invalid, "INVALID"},     //
    {12, op_syscall, "syscall"},  //
    {13, op_break, "break"},      //
    {14, invalid, "INVALID"},     //
    {15, invalid, "INVALID"},     //
                                  //
    {16, op_mfhi, "mfhi"},        //
    {17, op_mthi, "mthi"},        //
    {18, op_mflo, "mflo"},        //
    {19, op_mtlo, "mtlo"},        //
    {20, invalid, "INVALID"},     //
    {21, invalid, "INVALID"},     //
    {22, invalid, "INVALID"},     //
    {23, invalid, "INVALID"},     //
                                  //
    {24, op_mult, "mult"},        //
    {25, op_multu, "multu"},      //
    {26, op_div, "div"},          //
    {27, op_divu, "divu"},        //
    {28, invalid, "INVALID"},     //
    {29, invalid, "INVALID"},     //
    {30, invalid, "INVALID"},     //
    {31, invalid, "INVALID"},     //
                                  //
    {32, op_add, "add"},          //
    {33, op_addu, "addu"},        //
    {34, op_sub, "sub"},          //
    {35, op_subu, "subu"},        //
    {36, op_and, "and"},          //
    {37, op_or, "or"},            //
    {38, op_xor, "xor"},          //
    {39, op_nor, "nor"},          //
                                  //
    {40, invalid, "INVALID"},     //
    {41, invalid, "INVALID"},     //
    {42, op_slt, "slt"},          //
    {43, op_sltu, "sltu"},        //
    {44, invalid, "INVALID"},     //
    {45, invalid, "INVALID"},     //
    {46, invalid, "INVALID"},     //
    {47, invalid, "INVALID"},     //
                                  //
    {48, invalid, "INVALID"},     //
    {49, invalid, "INVALID"},     //
    {50, invalid, "INVALID"},     //
    {51, invalid, "INVALID"},     //
    {52, invalid, "INVALID"},     //
    {53, invalid, "INVALID"},     //
    {54, invalid, "INVALID"},     //
    {55, invalid, "INVALID"},     //
                                  //
    {56, invalid, "INVALID"},     //
    {57, invalid, "INVALID"},     //
    {58, invalid, "INVALID"},     //
    {59, invalid, "INVALID"},     //
    {60, invalid, "INVALID"},     //
    {61, invalid, "INVALID"},     //
    {62, invalid, "INVALID"},     //
    {63, invalid, "INVALID"},     //
};

void exception(mips::CPU *cpu, cop0::CAUSE::Exception cause) {
    cpu->cop0.cause.exception = cause;

    if (cpu->shouldJump) {
        cpu->cop0.cause.isInDelaySlot = true;
        cpu->cop0.epc = cpu->PC - 4;
    } else {
        cpu->cop0.cause.isInDelaySlot = false;
        cpu->cop0.epc = cpu->PC;
    }

    cpu->cop0.status.oldInterruptEnable = cpu->cop0.status.previousInterruptEnable;
    cpu->cop0.status.oldMode = cpu->cop0.status.previousMode;

    cpu->cop0.status.previousInterruptEnable = cpu->cop0.status.interruptEnable;
    cpu->cop0.status.previousMode = cpu->cop0.status.mode;

    cpu->cop0.status.interruptEnable = false;
    cpu->cop0.status.mode = cop0::STATUS::Mode::kernel;

    if (cpu->cop0.status.bootExceptionVectors == cop0::STATUS::BootExceptionVectors::rom) {
        cpu->PC = 0xbfc00180;
    } else {
        cpu->PC = 0x80000080;
    }

    cpu->shouldJump = false;
    cpu->exception = true;
}

void dummy(CPU *cpu, Opcode i) {}

void invalid(CPU *cpu, Opcode i) {
    printf("Invalid opcode (%s) at 0x%08x: 0x%08x\n", OpcodeTable[i.op].mnemnic, cpu->PC, i.opcode);
    cpu->state = CPU::State::halted;
}

void notImplemented(CPU *cpu, Opcode i) {
    printf("Opcode %s not implemented at 0x%08x: 0x%08x\n", OpcodeTable[i.op].mnemnic, cpu->PC, i.opcode);
    cpu->state = CPU::State::halted;
}

void special(CPU *cpu, Opcode i) {
    const auto &instruction = SpecialTable[i.fun];
    mnemonic(instruction.mnemnic);
    instruction.instruction(cpu, i);
}

// Shift Word Left Logical
// SLL rd, rt, a
void op_sll(CPU *cpu, Opcode i) {
    if (i.rt == 0 && i.rd == 0 && i.sh == 0) {
        mnemonic("nop");
        disasm(" ");
    } else {
        disasm("r%d, r%d, %d", i.rd, i.rt, i.sh);
        cpu->reg[i.rd] = cpu->reg[i.rt] << i.sh;
    }
}

// Shift Word Right Logical
// SRL rd, rt, a
void op_srl(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, %d", i.rd, i.rt, i.sh);
    cpu->reg[i.rd] = cpu->reg[i.rt] >> i.sh;
}

// Shift Word Right Arithmetic
// SRA rd, rt, a
void op_sra(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, %d", i.rd, i.rt, i.sh);
    cpu->reg[i.rd] = ((int32_t)cpu->reg[i.rt]) >> i.sh;
}

// Shift Word Left Logical Variable
// SLLV rd, rt, rs
void op_sllv(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rt, i.rs);
    cpu->reg[i.rd] = cpu->reg[i.rt] << (cpu->reg[i.rs] & 0x1f);
}

// Shift Word Right Logical Variable
// SRLV rd, rt, a
void op_srlv(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, %d", i.rd, i.rt, i.sh);
    cpu->reg[i.rd] = cpu->reg[i.rt] >> (cpu->reg[i.rs] & 0x1f);
}

// Shift Word Right Arithmetic Variable
// SRAV rd, rt, rs
void op_srav(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rt, i.rs);
    cpu->reg[i.rd] = ((int32_t)cpu->reg[i.rt]) >> (cpu->reg[i.rs] & 0x1f);
}

// Jump Register
// JR rs
void op_jr(CPU *cpu, Opcode i) {
    disasm("r%d", i.rs);
    uint32_t addr = cpu->reg[i.rs];
    if (addr & 3) {
        exception(cpu, cop0::CAUSE::Exception::addressErrorLoad);
        return;
    }
    cpu->shouldJump = true;
    cpu->jumpPC = addr;
}

// Jump Register
// JALR
void op_jalr(CPU *cpu, Opcode i) {
    disasm("r%d r%d", i.rd, i.rs);
    // cpu->loadDelaySlot(i.rd, cpu->PC + 8);
    uint32_t addr = cpu->reg[i.rs];
    if (addr & 3) {
        // TODO: not working correctly
        exception(cpu, cop0::CAUSE::Exception::addressErrorLoad);
        return;
    }
    cpu->shouldJump = true;
    cpu->jumpPC = addr;
    cpu->reg[i.rd] = cpu->PC + 8;
}

// Syscall
// SYSCALL
void op_syscall(CPU *cpu, Opcode i) {
    if (cpu->biosLog) printf("  SYSCALL(%d)\n", cpu->reg[4]);
    exception(cpu, cop0::CAUSE::Exception::syscall);
}

// Break
// BREAK
void op_break(CPU *cpu, Opcode i) { exception(cpu, cop0::CAUSE::Exception::breakpoint); }

// Move From Hi
// MFHI rd
void op_mfhi(CPU *cpu, Opcode i) {
    disasm("r%d", i.rd);
    cpu->reg[i.rd] = cpu->hi;
}

// Move To Hi
// MTHI rd
void op_mthi(CPU *cpu, Opcode i) {
    disasm("r%d", i.rs);
    cpu->hi = cpu->reg[i.rs];
}

// Move From Lo
// MFLO rd
void op_mflo(CPU *cpu, Opcode i) {
    disasm("r%d", i.rd);
    cpu->reg[i.rd] = cpu->lo;
}

// Move To Lo
// MTLO rd
void op_mtlo(CPU *cpu, Opcode i) {
    disasm("r%d", i.rs);
    cpu->lo = cpu->reg[i.rs];
}

// Multiply
// mult rs, rt
void op_mult(CPU *cpu, Opcode i) {
    disasm("r%d, r%d", i.rs, i.rt);
    uint64_t temp = (int64_t)(int32_t)cpu->reg[i.rs] * (int64_t)(int32_t)cpu->reg[i.rt];
    cpu->lo = temp & 0xffffffff;
    cpu->hi = temp >> 32;
}

// Multiply Unsigned
// multu rs, rt
void op_multu(CPU *cpu, Opcode i) {
    disasm("r%d, r%d", i.rs, i.rt);
    uint64_t temp = (uint64_t)cpu->reg[i.rs] * (uint64_t)cpu->reg[i.rt];
    cpu->lo = temp & 0xffffffff;
    cpu->hi = temp >> 32;
}
// Divide
// div rs, rt
void op_div(CPU *cpu, Opcode i) {
    disasm("r%d, r%d", i.rs, i.rt);

    int32_t rs = (int32_t)cpu->reg[i.rs];
    int32_t rt = (int32_t)cpu->reg[i.rt];
    if (rt == 0) {
        cpu->lo = (rs < 0) ? 0x00000001 : 0xffffffff;
        cpu->hi = rs;
    } else if ((uint32_t)rs == 0x80000000 && (uint32_t)rt == 0xffffffff) {
        cpu->lo = 0x80000000;
        cpu->hi = 0x00000000;
    } else {
        cpu->lo = rs / rt;
        cpu->hi = rs % rt;
    }
}

// Divide Unsigned Word
// divu rs, rt
void op_divu(CPU *cpu, Opcode i) {
    disasm("r%d, r%d", i.rs, i.rt);

    uint32_t rs = cpu->reg[i.rs];
    uint32_t rt = cpu->reg[i.rt];

    if (rt == 0) {
        cpu->lo = 0xffffffff;
        cpu->hi = rs;
    } else {
        cpu->lo = rs / rt;
        cpu->hi = rs % rt;
    }
}

// add rd, rs, rt
void op_add(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);

    uint32_t a = cpu->reg[i.rs];
    uint32_t b = cpu->reg[i.rt];
    uint32_t result = a + b;

    if (!((a ^ b) & 0x80000000) && ((result ^ a) & 0x80000000)) {
        exception(cpu, cop0::CAUSE::Exception::arithmeticOverflow);
        return;
    }
    cpu->reg[i.rd] = result;
}

// Add unsigned
// addu rd, rs, rt
void op_addu(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
    cpu->reg[i.rd] = cpu->reg[i.rs] + cpu->reg[i.rt];
}

// Subtract
// sub rd, rs, rt
void op_sub(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
    uint32_t a = cpu->reg[i.rs];
    uint32_t b = cpu->reg[i.rt];
    uint32_t result = a - b;

    if (((a ^ b) & 0x80000000) && ((result ^ a) & 0x80000000)) {
        exception(cpu, cop0::CAUSE::Exception::arithmeticOverflow);
        return;
    }
    cpu->reg[i.rd] = result;
}

// Subtract unsigned
// subu rd, rs, rt
void op_subu(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
    cpu->reg[i.rd] = cpu->reg[i.rs] - cpu->reg[i.rt];
}

// And
// and rd, rs, rt
void op_and(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
    cpu->reg[i.rd] = cpu->reg[i.rs] & cpu->reg[i.rt];
}

// Or
// OR rd, rs, rt
void op_or(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
    cpu->reg[i.rd] = cpu->reg[i.rs] | cpu->reg[i.rt];
}

// Xor
// XOR rd, rs, rt
void op_xor(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
    cpu->reg[i.rd] = cpu->reg[i.rs] ^ cpu->reg[i.rt];
}

// Nor
// NOR rd, rs, rt
void op_nor(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
    cpu->reg[i.rd] = ~(cpu->reg[i.rs] | cpu->reg[i.rt]);
}

// Set On Less Than Signed
// SLT rd, rs, rt
void op_slt(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
    if ((int32_t)cpu->reg[i.rs] < (int32_t)cpu->reg[i.rt])
        cpu->reg[i.rd] = 1;
    else
        cpu->reg[i.rd] = 0;
}

// Set On Less Than Unsigned
// SLTU rd, rs, rt
void op_sltu(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
    if (cpu->reg[i.rs] < cpu->reg[i.rt])
        cpu->reg[i.rd] = 1;
    else
        cpu->reg[i.rd] = 0;
}

void branch(CPU *cpu, Opcode i) {
    switch (i.rt & 0x11) {
        case 0:
            // Branch On Less Than Zero
            // BLTZ rs, offset
            mnemonic("bltz");
            disasm("r%d, %d", i.rs, i.offset);

            if ((int32_t)cpu->reg[i.rs] < 0) {
                cpu->shouldJump = true;
                cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset * 4);
            }
            break;

        case 1:
            // Branch On Greater Than Or Equal To Zero
            // BGEZ rs, offset
            mnemonic("bgez");
            disasm("r%d, %d", i.rs, i.offset);

            if ((int32_t)cpu->reg[i.rs] >= 0) {
                cpu->shouldJump = true;
                cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset * 4);
            }
            break;

        case 16:
            // Branch On Less Than Zero And Link
            // bltzal rs, offset
            mnemonic("bltzal");
            disasm("r%d, %d", i.rs, i.offset);

            cpu->reg[31] = cpu->PC + 8;
            // cpu->loadDelaySlot(31, cpu->PC + 8);
            if ((int32_t)cpu->reg[i.rs] < 0) {
                cpu->shouldJump = true;
                cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset * 4);
            }
            break;

        case 17:
            // Branch On Greater Than Or Equal To Zero And Link
            // BGEZAL rs, offset
            mnemonic("bgezal");
            disasm("r%d, %d", i.rs, i.offset);

            cpu->reg[31] = cpu->PC + 8;
            // cpu->loadDelaySlot(31, cpu->PC + 8);
            if ((int32_t)cpu->reg[i.rs] >= 0) {
                cpu->shouldJump = true;
                cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset * 4);
            }
            break;

        default:
            invalid(cpu, i);
    }
}

// Jump
// J target
void op_j(CPU *cpu, Opcode i) {
    disasm("0x%x", i.target * 4);
    cpu->shouldJump = true;
    cpu->jumpPC = (cpu->PC & 0xf0000000) | (i.target * 4);
}

// Jump And Link
// JAL target
void op_jal(CPU *cpu, Opcode i) {
    disasm("0x%x", (i.target * 4));
    cpu->shouldJump = true;
    cpu->jumpPC = (cpu->PC & 0xf0000000) | (i.target * 4);
    cpu->reg[31] = cpu->PC + 8;
}

// Branch On Equal
// BEQ rs, rt, offset
void op_beq(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, %d", i.rs, i.rt, i.offset);
    if (cpu->reg[i.rt] == cpu->reg[i.rs]) {
        cpu->shouldJump = true;
        cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset * 4);
    }
}

// Branch On Greater Than Zero
// BGTZ rs, offset
void op_bgtz(CPU *cpu, Opcode i) {
    disasm("r%d, %d", i.rs, i.offset);
    if ((int32_t)cpu->reg[i.rs] > 0) {
        cpu->shouldJump = true;
        cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset * 4);
    }
}

// Branch On Less Than Or Equal To Zero
// BLEZ rs, offset
void op_blez(CPU *cpu, Opcode i) {
    disasm("r%d, %d", i.rs, i.offset);
    if ((int32_t)cpu->reg[i.rs] <= 0) {
        cpu->shouldJump = true;
        cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset * 4);
    }
}

// Branch On Not Equal
// BNE rs, offset
void op_bne(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, %d", i.rs, i.rt, i.offset);
    if (cpu->reg[i.rt] != cpu->reg[i.rs]) {
        cpu->shouldJump = true;
        cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset * 4);
    }
}

// Add Immediate Word
// ADDI rt, rs, imm
void op_addi(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, 0x%hx", i.rt, i.rs, i.offset);
    uint32_t a = cpu->reg[i.rs];
    uint32_t b = i.offset;
    uint32_t result = a + b;

    if (!((a ^ b) & 0x80000000) && ((result ^ a) & 0x80000000)) {
        exception(cpu, cop0::CAUSE::Exception::arithmeticOverflow);
        return;
    }
    cpu->reg[i.rt] = result;
}

// Add Immediate Unsigned Word
// ADDIU rt, rs, imm
void op_addiu(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, 0x%hx", i.rt, i.rs, i.offset);
    cpu->reg[i.rt] = cpu->reg[i.rs] + i.offset;
}

// Set On Less Than Immediate
// SLTI rd, rs, rt
void op_slti(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, 0x%x", i.rt, i.rs, i.imm);
    if ((int32_t)cpu->reg[i.rs] < (int32_t)i.offset)
        cpu->reg[i.rt] = 1;
    else
        cpu->reg[i.rt] = 0;
}

// Set On Less Than Immediate Unsigned
// SLTIU rd, rs, rt
void op_sltiu(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, 0x%x", i.rt, i.rs, i.imm);
    if (cpu->reg[i.rs] < (uint32_t)i.offset)
        cpu->reg[i.rt] = 1;
    else
        cpu->reg[i.rt] = 0;
}

// And Immediate
// ANDI rt, rs, imm
void op_andi(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, 0x%x", i.rt, i.rs, i.imm);
    cpu->reg[i.rt] = cpu->reg[i.rs] & i.imm;
}

// Or Immediete
// ORI rt, rs, imm
void op_ori(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, 0x%x", i.rt, i.rs, i.imm);
    cpu->reg[i.rt] = cpu->reg[i.rs] | i.imm;
}

// Xor Immediate
// XORI rt, rs, imm
void op_xori(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, 0x%x", i.rt, i.rs, i.imm);
    cpu->reg[i.rt] = cpu->reg[i.rs] ^ i.imm;
}

// Load Upper Immediate
// LUI rt, imm
void op_lui(CPU *cpu, Opcode i) {
    disasm("r%d, 0x%x", i.rt, i.imm);
    cpu->reg[i.rt] = i.imm << 16;
}

// Coprocessor zero
void op_cop0(CPU *cpu, Opcode i) {
    switch (i.rs) {
        case 0:
            // Move from co-processor zero
            // MFC0 rd, <nn>
            mnemonic("MFC0");
            disasm("r%d, $%d", i.rt, i.rd);

            switch (i.rd) {
                case 3:
                    cpu->reg[i.rt] = cpu->cop0.bpc;
                    break;

                case 7:
                    cpu->reg[i.rt] = cpu->cop0.dcic;
                    break;

                case 8:
                    cpu->reg[i.rt] = cpu->cop0.badVaddr;
                    break;

                case 12:
                    cpu->reg[i.rt] = cpu->cop0.status._reg;
                    break;

                case 13:
                    cpu->reg[i.rt] = cpu->cop0.cause._reg;
                    break;

                case 14:
                    cpu->reg[i.rt] = cpu->cop0.epc;
                    break;

                case 15:
                    cpu->reg[i.rt] = cpu->cop0.revId;
                    break;

                default:
                    cpu->reg[i.rt] = 0;
                    break;
            }

            break;

        case 4:
            // Move to co-processor zero
            // MTC0 rs, <nn>
            mnemonic("MTC0");
            disasm("r%d, $%d", i.rt, i.rd);

            switch (i.rd) {
                case 3:
                    cpu->cop0.bpc = cpu->reg[i.rt];
                    break;

                case 7:
                    cpu->cop0.dcic = cpu->reg[i.rt];
                    break;

                case 12:
                    if (cpu->reg[i.rt] & (1 << 17)) {
                        // printf("Panic, SwC not handled\n");
                        // cpu->state = CPU::State::halted;
                    }
                    cpu->cop0.status._reg = cpu->reg[i.rt];
                    break;

                case 13:
                    cpu->cop0.cause._reg &= ~0x300;
                    cpu->cop0.cause._reg |= (cpu->reg[i.rt] & 0x300);
                    break;

                default:
                    cpu->reg[i.rt] = 0;
                    break;
            }
            break;

        case 16:
            // Restore from exception
            // RFE
            mnemonic("RFE");

            cpu->cop0.status.interruptEnable = cpu->cop0.status.previousInterruptEnable;
            cpu->cop0.status.mode = cpu->cop0.status.previousMode;

            cpu->cop0.status.previousInterruptEnable = cpu->cop0.status.oldInterruptEnable;
            cpu->cop0.status.previousMode = cpu->cop0.status.oldMode;
            break;

        default:
            invalid(cpu, i);
    }
}

// Coprocessor two
void op_cop2(CPU *cpu, Opcode i) { /*printf("COP2: 0x%08x\n", i.opcode);*/
    if (i.opcode & 0x3f == 0x00) {
        if (i.rs == 0x00) {
            // Move data from co-processor two
            // MFC2 rt, <nn>
            mnemonic("MFC2");
            disasm("r%d, $%d", i.rt, i.rd);
            cpu->reg[i.rt] = cpu->gte.read(i.rd);
        } else if (i.rs == 0x02) {
            // Move control from co-processor two
            // CFC2 rt, <nn>
            mnemonic("MFC2");
            disasm("r%d, $%d", i.rt, i.rd);
            cpu->reg[i.rt] = cpu->gte.read(i.rd + 32);
        } else if (i.rs == 0x04) {
            // Move data to co-processor two
            // MTC2 rt, <nn>
            mnemonic("MTC2");
            disasm("r%d, $%d", i.rt, i.rd);
            cpu->gte.write(i.rd, cpu->reg[i.rt]);
        } else if (i.rs == 0x06) {
            // Move control to co-processor two
            // CTC2 rt, <nn>
            mnemonic("MTC2");
            disasm("r%d, $%d", i.rt, i.rd);
            cpu->gte.write(i.rd + 32, cpu->reg[i.rt]);
        } else {
            invalid(cpu, i);
        }
    } else {
        gte::Command command(i.opcode);
        disasm("0x%x  #opcode = 0x%x", command._reg, command.cmd);

        switch (command.cmd) {
            case 0x06:
                cpu->gte.nclip();
                return;
            case 0x13:
                cpu->gte.ncds();
                return;
            case 0x30:
                cpu->gte.rtpt(command.sf);
                return;
            default:
                // printf("Unhandled gte command 0x%x\n", opcode);
                // cpu->state = CPU::State::halted;
                return;
        }
    }
}

// Load Byte
// LB rt, offset(base)
void op_lb(CPU *cpu, Opcode i) {
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    disasm("r%d, 0x%hx(r%d)  r%d = 0x%08x", i.rt, i.offset, i.rs, i.rs, addr);
    cpu->loadDelaySlot(i.rt, ((int32_t)(cpu->readMemory8(addr) << 24)) >> 24);
}

// Load Halfword
// LH rt, offset(base)
void op_lh(CPU *cpu, Opcode i) {
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    disasm("r%d, 0x%hx(r%d)  r%d = 0x%08x", i.rt, i.offset, i.rs, i.rs, addr);
    if (addr & 1)  // non aligned address
    {
        exception(cpu, cop0::CAUSE::Exception::addressErrorLoad);
        return;
    }
    cpu->loadDelaySlot(i.rt, (int32_t)(int16_t)cpu->readMemory16(addr));
}

// Load Word Left
// LWL rt, offset(base)
void op_lwl(CPU *cpu, Opcode i) {
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    disasm("r%d, %hx(r%d)  r%d = 0x%08x", i.rt, i.offset, i.rs, i.rs, addr);

    uint32_t mem = cpu->readMemory32(addr & 0xfffffffc);

    uint32_t reg;
    if (cpu->slots[0].reg == i.rt) {
        reg = cpu->slots[0].data;
    } else {
        reg = cpu->reg[i.rt];
    }

    uint32_t result = 0;
    switch (addr % 4) {
        case 0:
            result = (reg & 0x00ffffff) | (mem << 24);
            break;
        case 1:
            result = (reg & 0x0000ffff) | (mem << 16);
            break;
        case 2:
            result = (reg & 0x000000ff) | (mem << 8);
            break;
        case 3:
            result = (reg & 0x00000000) | (mem);
            break;
    }
    cpu->loadDelaySlot(i.rt, result);
}

// Load Word
// LW rt, offset(base)
void op_lw(CPU *cpu, Opcode i) {
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    disasm("r%d, 0x%hx(r%d)  r%d = 0x%08x", i.rt, i.offset, i.rs, i.rs, addr);
    if (addr & 3)  // non aligned address
    {
        exception(cpu, cop0::CAUSE::Exception::addressErrorLoad);
        return;
    }
    cpu->loadDelaySlot(i.rt, cpu->readMemory32(addr));
}

// Load Byte Unsigned
// LBU rt, offset(base)
void op_lbu(CPU *cpu, Opcode i) {
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    disasm("r%d, %hx(r%d)  r%d = 0x%08x", i.rt, i.offset, i.rs, i.rs, addr);
    cpu->loadDelaySlot(i.rt, cpu->readMemory8(addr));
}

// Load Halfword Unsigned
// LHU rt, offset(base)
void op_lhu(CPU *cpu, Opcode i) {
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    disasm("r%d, %hx(r%d)  r%d = 0x%08x", i.rt, i.offset, i.rs, i.rs, addr);
    if (addr & 1)  // non aligned address
    {
        exception(cpu, cop0::CAUSE::Exception::addressErrorLoad);
        return;
    }
    cpu->loadDelaySlot(i.rt, cpu->readMemory16(addr));
}

// Load Word Right
// LWR rt, offset(base)
void op_lwr(CPU *cpu, Opcode i) {
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    disasm("r%d, %hx(r%d)  r%d = 0x%08x", i.rt, i.offset, i.rs, i.rs, addr);

    uint32_t mem = cpu->readMemory32(addr & 0xfffffffc);

    uint32_t reg;
    if (cpu->slots[0].reg == i.rt) {
        reg = cpu->slots[0].data;
    } else {
        reg = cpu->reg[i.rt];
    }

    uint32_t result = 0;
    switch (addr % 4) {
        case 0:
            result = (reg & 0x00000000) | (mem);
            break;
        case 1:
            result = (reg & 0xff000000) | (mem >> 8);
            break;
        case 2:
            result = (reg & 0xffff0000) | (mem >> 16);
            break;
        case 3:
            result = (reg & 0xffffff00) | (mem >> 24);
            break;
    }
    cpu->loadDelaySlot(i.rt, result);
}

// Store Byte
// SB rt, offset(base)
void op_sb(CPU *cpu, Opcode i) {
    disasm("r%d, %hx(r%d)", i.rt, i.offset, i.rs);
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    cpu->writeMemory8(addr, cpu->reg[i.rt]);
}

// Store Halfword
// SH rt, offset(base)
void op_sh(CPU *cpu, Opcode i) {
    disasm("r%d, %hx(r%d)", i.rt, i.offset, i.rs);
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    if (addr & 1)  // non aligned address
    {
        exception(cpu, cop0::CAUSE::Exception::addressErrorStore);
        return;
    }
    cpu->writeMemory16(addr, cpu->reg[i.rt]);
}

// Store Word Left
// SWL rt, offset(base)
void op_swl(CPU *cpu, Opcode i) {
    disasm("r%d, %hx(r%d)", i.rt, i.offset, i.rs);

    uint32_t addr = cpu->reg[i.rs] + i.offset;
    uint32_t mem = cpu->readMemory32(addr & 0xfffffffc);
    uint32_t reg = cpu->reg[i.rt];

    uint32_t result = 0;
    switch (addr % 4) {
        case 0:
            result = (mem & 0xffffff00) | (reg >> 24);
            break;
        case 1:
            result = (mem & 0xffff0000) | (reg >> 16);
            break;
        case 2:
            result = (mem & 0xff000000) | (reg >> 8);
            break;
        case 3:
            result = (mem & 0x00000000) | (reg);
            break;
    }
    cpu->writeMemory32(addr & 0xfffffffc, result);
}

// Store Word
// SW rt, offset(base)
void op_sw(CPU *cpu, Opcode i) {
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    disasm("r%d, %hx(r%d)  r%d = 0x%08x", i.rt, i.offset, i.rs, i.rs, addr);
    if (addr & 3)  // non aligned address
    {
        exception(cpu, cop0::CAUSE::Exception::addressErrorStore);
        return;
    }
    cpu->writeMemory32(addr, cpu->reg[i.rt]);
}

// Store Word Right
// SWR rt, offset(base)
void op_swr(CPU *cpu, Opcode i) {
    disasm("r%d, %hx(r%d)", i.rt, i.offset, i.rs);

    uint32_t addr = cpu->reg[i.rs] + i.offset;
    uint32_t mem = cpu->readMemory32(addr & 0xfffffffc);
    uint32_t reg = cpu->reg[i.rt];

    uint32_t result = 0;
    switch (addr % 4) {
        case 0:
            result = (reg) | (mem & 0x00000000);
            break;
        case 1:
            result = (reg << 8) | (mem & 0x000000ff);
            break;
        case 2:
            result = (reg << 16) | (mem & 0x0000ffff);
            break;
        case 3:
            result = (reg << 24) | (mem & 0x00ffffff);
            break;
    }
    cpu->writeMemory32(addr & 0xfffffffc, result);
}

// Load to coprocessor 2
// LWC2 ??? ???
void op_lwc2(CPU *cpu, Opcode i) {
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    disasm("%d, %hx(r%d)  #addr = 0x%08x", i.rt, i.offset, i.rs, addr);

    assert(i.rt < 64);
    cpu->gte.write(i.rt, cpu->readMemory32(addr));
}

// Store from coprocessor 2
// SWC2 ??? ???
void op_swc2(CPU *cpu, Opcode i) {
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    disasm("%d, %hx(r%d)  #addr = 0x%08x", i.rt, i.offset, i.rs, addr);

    assert(i.rt < 64);
    cpu->writeMemory32(addr, cpu->gte.read(i.rt));
}

// BREAKPOINT
void op_breakpoint(CPU *cpu, Opcode i) {
    disasm("");
    cpu->state = CPU::State::halted;
    cpu->PC += 4;
}
};
