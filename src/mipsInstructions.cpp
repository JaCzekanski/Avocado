#include "mipsInstructions.h"
#include "mips.h"
#include "utils/string.h"

using namespace mips;

namespace mipsInstructions {

// clang-format off
PrimaryInstruction OpcodeTable[64] = {
    {0, special},
    {1, branch},
    {2, op_j},
    {3, op_jal},
    {4, op_beq},
    {5, op_bne},
    {6, op_blez},
    {7, op_bgtz},

    {8, op_addi},
    {9, op_addiu},
    {10, op_slti},
    {11, op_sltiu},
    {12, op_andi},
    {13, op_ori},
    {14, op_xori},
    {15, op_lui},

    {16, op_cop0},
    {17, notImplemented},
    {18, op_cop2},
    {19, notImplemented},
    {20, invalid},
    {21, invalid},
    {22, invalid},
    {23, invalid},

    {24, invalid},
    {25, invalid},
    {26, invalid},
    {27, invalid},
    {28, invalid},
    {29, invalid},
    {30, invalid},
    {31, invalid},

    {32, op_lb},
    {33, op_lh},
    {34, op_lwl},
    {35, op_lw},
    {36, op_lbu},
    {37, op_lhu},
    {38, op_lwr},
    {39, invalid},

    {40, op_sb},
    {41, op_sh},
    {42, op_swl},
    {43, op_sw},
    {44, invalid},
    {45, invalid},
    {46, op_swr},
    {47, invalid},

    {48, notImplemented},
    {49, notImplemented},
    {50, op_lwc2},
    {51, notImplemented},
    {52, invalid},
    {53, invalid},
    {54, invalid},
    {55, invalid},

    {56, notImplemented},
    {57, notImplemented},
    {58, op_swc2},
    {59, notImplemented},
    {60, invalid},
    {61, invalid},
    {62, invalid},
#ifdef ENABLE_BREAKPOINTS
    {63, op_breakpoint}
#else
    {63, invalid}
#endif
};

PrimaryInstruction SpecialTable[64] = {
    // opcodes encoded with "function" field, when opcode == 0
    {0, op_sll},
    {1, invalid},
    {2, op_srl},
    {3, op_sra},
    {4, op_sllv},
    {5, invalid},
    {6, op_srlv},
    {7, op_srav},

    {8, op_jr},
    {9, op_jalr},
    {10, invalid},
    {11, invalid},
    {12, op_syscall},
    {13, op_break},
    {14, invalid},
    {15, invalid},

    {16, op_mfhi},
    {17, op_mthi},
    {18, op_mflo},
    {19, op_mtlo},
    {20, invalid},
    {21, invalid},
    {22, invalid},
    {23, invalid},

    {24, op_mult},
    {25, op_multu},
    {26, op_div},
    {27, op_divu},
    {28, invalid},
    {29, invalid},
    {30, invalid},
    {31, invalid},

    {32, op_add},
    {33, op_addu},
    {34, op_sub},
    {35, op_subu},
    {36, op_and},
    {37, op_or},
    {38, op_xor},
    {39, op_nor},

    {40, invalid},
    {41, invalid},
    {42, op_slt},
    {43, op_sltu},
    {44, invalid},
    {45, invalid},
    {46, invalid},
    {47, invalid},

    {48, invalid},
    {49, invalid},
    {50, invalid},
    {51, invalid},
    {52, invalid},
    {53, invalid},
    {54, invalid},
    {55, invalid},

    {56, invalid},
    {57, invalid},
    {58, invalid},
    {59, invalid},
    {60, invalid},
    {61, invalid},
    {62, invalid},
    {63, invalid},
};
// clang-format on

void exception(CPU *cpu, COP0::CAUSE::Exception cause) {
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
    cpu->cop0.status.mode = COP0::STATUS::Mode::kernel;

    if (cpu->cop0.status.bootExceptionVectors == COP0::STATUS::BootExceptionVectors::rom) {
        cpu->PC = 0xbfc00180;
    } else {
        cpu->PC = 0x80000080;
    }

    cpu->shouldJump = false;
    cpu->exception = true;
}

void dummy(CPU *cpu, Opcode i) {}

void invalid(CPU *cpu, Opcode i) {
    printf("Invalid opcode at 0x%08x: 0x%08x\n", cpu->PC, i.opcode);
    cpu->state = CPU::State::halted;
}

void notImplemented(CPU *cpu, Opcode i) {
    printf("Opcode not implemented at 0x%08x: 0x%08x\n", cpu->PC, i.opcode);
    cpu->state = CPU::State::halted;
}

void special(CPU *cpu, Opcode i) {
    const auto &instruction = SpecialTable[i.fun];
    instruction.instruction(cpu, i);
}

// Shift Word Left Logical
// SLL rd, rt, a
void op_sll(CPU *cpu, Opcode i) {
    if (i.rt == 0 && i.rd == 0 && i.sh == 0) {
    } else {
        cpu->reg[i.rd] = cpu->reg[i.rt] << i.sh;
    }
}

// Shift Word Right Logical
// SRL rd, rt, a
void op_srl(CPU *cpu, Opcode i) { cpu->reg[i.rd] = cpu->reg[i.rt] >> i.sh; }

// Shift Word Right Arithmetic
// SRA rd, rt, a
void op_sra(CPU *cpu, Opcode i) { cpu->reg[i.rd] = ((int32_t)cpu->reg[i.rt]) >> i.sh; }

// Shift Word Left Logical Variable
// SLLV rd, rt, rs
void op_sllv(CPU *cpu, Opcode i) { cpu->reg[i.rd] = cpu->reg[i.rt] << (cpu->reg[i.rs] & 0x1f); }

// Shift Word Right Logical Variable
// SRLV rd, rt, a
void op_srlv(CPU *cpu, Opcode i) { cpu->reg[i.rd] = cpu->reg[i.rt] >> (cpu->reg[i.rs] & 0x1f); }

// Shift Word Right Arithmetic Variable
// SRAV rd, rt, rs
void op_srav(CPU *cpu, Opcode i) { cpu->reg[i.rd] = ((int32_t)cpu->reg[i.rt]) >> (cpu->reg[i.rs] & 0x1f); }

// Jump Register
// JR rs
void op_jr(CPU *cpu, Opcode i) {
    uint32_t addr = cpu->reg[i.rs];
    if (addr & 3) {
        exception(cpu, COP0::CAUSE::Exception::addressErrorLoad);
        return;
    }
    cpu->shouldJump = true;
    cpu->jumpPC = addr;
}

// Jump Register
// JALR
void op_jalr(CPU *cpu, Opcode i) {
    // cpu->loadDelaySlot(i.rd, cpu->PC + 8);
    uint32_t addr = cpu->reg[i.rs];
    if (addr & 3) {
        // TODO: not working correctly
        exception(cpu, COP0::CAUSE::Exception::addressErrorLoad);
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
    exception(cpu, COP0::CAUSE::Exception::syscall);
}

// Break
// BREAK
void op_break(CPU *cpu, Opcode i) { exception(cpu, COP0::CAUSE::Exception::breakpoint); }

// Move From Hi
// MFHI rd
void op_mfhi(CPU *cpu, Opcode i) { cpu->reg[i.rd] = cpu->hi; }

// Move To Hi
// MTHI rd
void op_mthi(CPU *cpu, Opcode i) { cpu->hi = cpu->reg[i.rs]; }

// Move From Lo
// MFLO rd
void op_mflo(CPU *cpu, Opcode i) { cpu->reg[i.rd] = cpu->lo; }

// Move To Lo
// MTLO rd
void op_mtlo(CPU *cpu, Opcode i) { cpu->lo = cpu->reg[i.rs]; }

// Multiply
// mult rs, rt
void op_mult(CPU *cpu, Opcode i) {
    uint64_t temp = (int64_t)(int32_t)cpu->reg[i.rs] * (int64_t)(int32_t)cpu->reg[i.rt];
    cpu->lo = temp & 0xffffffff;
    cpu->hi = temp >> 32;
}

// Multiply Unsigned
// multu rs, rt
void op_multu(CPU *cpu, Opcode i) {
    uint64_t temp = (uint64_t)cpu->reg[i.rs] * (uint64_t)cpu->reg[i.rt];
    cpu->lo = temp & 0xffffffff;
    cpu->hi = temp >> 32;
}
// Divide
// div rs, rt
void op_div(CPU *cpu, Opcode i) {
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
    uint32_t a = cpu->reg[i.rs];
    uint32_t b = cpu->reg[i.rt];
    uint32_t result = a + b;

    if (!((a ^ b) & 0x80000000) && ((result ^ a) & 0x80000000)) {
        exception(cpu, COP0::CAUSE::Exception::arithmeticOverflow);
        return;
    }
    cpu->reg[i.rd] = result;
}

// Add unsigned
// addu rd, rs, rt
void op_addu(CPU *cpu, Opcode i) { cpu->reg[i.rd] = cpu->reg[i.rs] + cpu->reg[i.rt]; }

// Subtract
// sub rd, rs, rt
void op_sub(CPU *cpu, Opcode i) {
    uint32_t a = cpu->reg[i.rs];
    uint32_t b = cpu->reg[i.rt];
    uint32_t result = a - b;

    if (((a ^ b) & 0x80000000) && ((result ^ a) & 0x80000000)) {
        exception(cpu, COP0::CAUSE::Exception::arithmeticOverflow);
        return;
    }
    cpu->reg[i.rd] = result;
}

// Subtract unsigned
// subu rd, rs, rt
void op_subu(CPU *cpu, Opcode i) { cpu->reg[i.rd] = cpu->reg[i.rs] - cpu->reg[i.rt]; }

// And
// and rd, rs, rt
void op_and(CPU *cpu, Opcode i) { cpu->reg[i.rd] = cpu->reg[i.rs] & cpu->reg[i.rt]; }

// Or
// OR rd, rs, rt
void op_or(CPU *cpu, Opcode i) { cpu->reg[i.rd] = cpu->reg[i.rs] | cpu->reg[i.rt]; }

// Xor
// XOR rd, rs, rt
void op_xor(CPU *cpu, Opcode i) { cpu->reg[i.rd] = cpu->reg[i.rs] ^ cpu->reg[i.rt]; }

// Nor
// NOR rd, rs, rt
void op_nor(CPU *cpu, Opcode i) { cpu->reg[i.rd] = ~(cpu->reg[i.rs] | cpu->reg[i.rt]); }

// Set On Less Than Signed
// SLT rd, rs, rt
void op_slt(CPU *cpu, Opcode i) {
    if ((int32_t)cpu->reg[i.rs] < (int32_t)cpu->reg[i.rt])
        cpu->reg[i.rd] = 1;
    else
        cpu->reg[i.rd] = 0;
}

// Set On Less Than Unsigned
// SLTU rd, rs, rt
void op_sltu(CPU *cpu, Opcode i) {
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
            if ((int32_t)cpu->reg[i.rs] < 0) {
                cpu->shouldJump = true;
                cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset * 4);
            }
            break;

        case 1:
            // Branch On Greater Than Or Equal To Zero
            // BGEZ rs, offset
            if ((int32_t)cpu->reg[i.rs] >= 0) {
                cpu->shouldJump = true;
                cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset * 4);
            }
            break;

        case 16:
            // Branch On Less Than Zero And Link
            // bltzal rs, offset
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
    cpu->shouldJump = true;
    cpu->jumpPC = (cpu->PC & 0xf0000000) | (i.target * 4);
}

// Jump And Link
// JAL target
void op_jal(CPU *cpu, Opcode i) {
    cpu->shouldJump = true;
    cpu->jumpPC = (cpu->PC & 0xf0000000) | (i.target * 4);
    cpu->reg[31] = cpu->PC + 8;
}

// Branch On Equal
// BEQ rs, rt, offset
void op_beq(CPU *cpu, Opcode i) {
    if (cpu->reg[i.rt] == cpu->reg[i.rs]) {
        cpu->shouldJump = true;
        cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset * 4);
    }
}

// Branch On Greater Than Zero
// BGTZ rs, offset
void op_bgtz(CPU *cpu, Opcode i) {
    if ((int32_t)cpu->reg[i.rs] > 0) {
        cpu->shouldJump = true;
        cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset * 4);
    }
}

// Branch On Less Than Or Equal To Zero
// BLEZ rs, offset
void op_blez(CPU *cpu, Opcode i) {
    if ((int32_t)cpu->reg[i.rs] <= 0) {
        cpu->shouldJump = true;
        cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset * 4);
    }
}

// Branch On Not Equal
// BNE rs, offset
void op_bne(CPU *cpu, Opcode i) {
    if (cpu->reg[i.rt] != cpu->reg[i.rs]) {
        cpu->shouldJump = true;
        cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset * 4);
    }
}

// Add Immediate Word
// ADDI rt, rs, imm
void op_addi(CPU *cpu, Opcode i) {
    uint32_t a = cpu->reg[i.rs];
    uint32_t b = i.offset;
    uint32_t result = a + b;

    if (!((a ^ b) & 0x80000000) && ((result ^ a) & 0x80000000)) {
        exception(cpu, COP0::CAUSE::Exception::arithmeticOverflow);
        return;
    }
    cpu->reg[i.rt] = result;
}

// Add Immediate Unsigned Word
// ADDIU rt, rs, imm
void op_addiu(CPU *cpu, Opcode i) { cpu->reg[i.rt] = cpu->reg[i.rs] + i.offset; }

// Set On Less Than Immediate
// SLTI rd, rs, rt
void op_slti(CPU *cpu, Opcode i) {
    if ((int32_t)cpu->reg[i.rs] < (int32_t)i.offset)
        cpu->reg[i.rt] = 1;
    else
        cpu->reg[i.rt] = 0;
}

// Set On Less Than Immediate Unsigned
// SLTIU rd, rs, rt
void op_sltiu(CPU *cpu, Opcode i) {
    if (cpu->reg[i.rs] < (uint32_t)i.offset)
        cpu->reg[i.rt] = 1;
    else
        cpu->reg[i.rt] = 0;
}

// And Immediate
// ANDI rt, rs, imm
void op_andi(CPU *cpu, Opcode i) { cpu->reg[i.rt] = cpu->reg[i.rs] & i.imm; }

// Or Immediete
// ORI rt, rs, imm
void op_ori(CPU *cpu, Opcode i) { cpu->reg[i.rt] = cpu->reg[i.rs] | i.imm; }

// Xor Immediate
// XORI rt, rs, imm
void op_xori(CPU *cpu, Opcode i) { cpu->reg[i.rt] = cpu->reg[i.rs] ^ i.imm; }

// Load Upper Immediate
// LUI rt, imm
void op_lui(CPU *cpu, Opcode i) { cpu->reg[i.rt] = i.imm << 16; }

// Coprocessor zero
void op_cop0(CPU *cpu, Opcode i) {
    switch (i.rs) {
        case 0:
            // Move from co-processor zero
            // MFC0 rd, <nn>
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
void op_cop2(CPU *cpu, Opcode i) {
    gte::Command command(i.opcode);
    if (command.cmd != 0x00) {
        cpu->gte.log.push_back({gte::GTE::GTE_ENTRY::MODE::func, command.cmd});

        if (!cpu->gte.command(command)) {
            printf("Unhandled gte command 0x%x\n", command.cmd);
        }
        return;
    }

    if (i.rs == 0x00) {
        // Move data from co-processor two
        // MFC2 rt, <nn>
        cpu->reg[i.rt] = cpu->gte.read(i.rd);
        cpu->gte.log.push_back({gte::GTE::GTE_ENTRY::MODE::read, (uint32_t)i.rd, cpu->reg[i.rt]});
    } else if (i.rs == 0x02) {
        // Move control from co-processor two
        // CFC2 rt, <nn>
        cpu->reg[i.rt] = cpu->gte.read(i.rd + 32);
        cpu->gte.log.push_back({gte::GTE::GTE_ENTRY::MODE::read, (uint32_t)(i.rd + 32), cpu->reg[i.rt]});
    } else if (i.rs == 0x04) {
        // Move data to co-processor two
        // MTC2 rt, <nn>
        cpu->gte.write(i.rd, cpu->reg[i.rt]);
    } else if (i.rs == 0x06) {
        // Move control to co-processor two
        // CTC2 rt, <nn>
        cpu->gte.write(i.rd + 32, cpu->reg[i.rt]);
    } else {
        invalid(cpu, i);
    }
}

// Load Byte
// LB rt, offset(base)
void op_lb(CPU *cpu, Opcode i) {
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    cpu->loadDelaySlot(i.rt, ((int32_t)(cpu->readMemory8(addr) << 24)) >> 24);
}

// Load Halfword
// LH rt, offset(base)
void op_lh(CPU *cpu, Opcode i) {
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    if (addr & 1)  // non aligned address
    {
        exception(cpu, COP0::CAUSE::Exception::addressErrorLoad);
        return;
    }
    cpu->loadDelaySlot(i.rt, (int32_t)(int16_t)cpu->readMemory16(addr));
}

// Load Word Left
// LWL rt, offset(base)
void op_lwl(CPU *cpu, Opcode i) {
    uint32_t addr = cpu->reg[i.rs] + i.offset;
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
    if (addr & 3)  // non aligned address
    {
        exception(cpu, COP0::CAUSE::Exception::addressErrorLoad);
        return;
    }
    cpu->loadDelaySlot(i.rt, cpu->readMemory32(addr));
}

// Load Byte Unsigned
// LBU rt, offset(base)
void op_lbu(CPU *cpu, Opcode i) {
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    cpu->loadDelaySlot(i.rt, cpu->readMemory8(addr));
}

// Load Halfword Unsigned
// LHU rt, offset(base)
void op_lhu(CPU *cpu, Opcode i) {
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    if (addr & 1)  // non aligned address
    {
        exception(cpu, COP0::CAUSE::Exception::addressErrorLoad);
        return;
    }
    cpu->loadDelaySlot(i.rt, cpu->readMemory16(addr));
}

// Load Word Right
// LWR rt, offset(base)
void op_lwr(CPU *cpu, Opcode i) {
    uint32_t addr = cpu->reg[i.rs] + i.offset;

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
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    cpu->writeMemory8(addr, cpu->reg[i.rt]);
}

// Store Halfword
// SH rt, offset(base)
void op_sh(CPU *cpu, Opcode i) {
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    if (addr & 1)  // non aligned address
    {
        exception(cpu, COP0::CAUSE::Exception::addressErrorStore);
        return;
    }
    cpu->writeMemory16(addr, cpu->reg[i.rt]);
}

// Store Word Left
// SWL rt, offset(base)
void op_swl(CPU *cpu, Opcode i) {
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
    if (addr & 3)  // non aligned address
    {
        exception(cpu, COP0::CAUSE::Exception::addressErrorStore);
        return;
    }
    cpu->writeMemory32(addr, cpu->reg[i.rt]);
}

// Store Word Right
// SWR rt, offset(base)
void op_swr(CPU *cpu, Opcode i) {
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

    assert(i.rt < 64);
    auto data = cpu->readMemory32(addr);
    cpu->gte.write(i.rt, data);
    cpu->gte.log.push_back({gte::GTE::GTE_ENTRY::MODE::write, (uint32_t)i.rt, data});
}

// Store from coprocessor 2
// SWC2 ??? ???
void op_swc2(CPU *cpu, Opcode i) {
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    assert(i.rt < 64);
    auto gteRead = cpu->gte.read(i.rt);
    cpu->writeMemory32(addr, gteRead);
    cpu->gte.log.push_back({gte::GTE::GTE_ENTRY::MODE::read, (uint32_t)i.rt, gteRead});
}

// BREAKPOINT
void op_breakpoint(CPU *cpu, Opcode i) {
    cpu->state = CPU::State::halted;
    cpu->PC += 4;
}
};
