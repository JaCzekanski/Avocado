#include "mipsInstructions.h"
#include "mips.h"
#include <string>
#include "utils/string.h"

#define mnemonic(x) \
    if (disassemblyEnabled) _mnemonic = x
#define disasm(fmt, ...) \
    if (disassemblyEnabled) _disasm = string_format(fmt, ##__VA_ARGS__)

using namespace mips;

extern bool disassemblyEnabled;
extern char *_mnemonic;
extern std::string _disasm;

namespace mipsInstructions {
PrimaryInstruction OpcodeTable[64] = {
    // R type
    {0, special, "special"},
    {1, branch, "branch"},
    {2, j, "j"},
    {3, jal, "jal"},
    {4, beq, "beq"},
    {5, bne, "bne"},
    {6, blez, "blez"},
    {7, bgtz, "bgtz"},

    {8, addi, "addi"},
    {9, addiu, "addiu"},
    {10, slti, "slti"},
    {11, sltiu, "sltiu"},
    {12, andi, "andi"},
    {13, ori, "ori"},
    {14, xori, "xori"},
    {15, lui, "lui"},

    {16, cop0, "cop0"},
    {17, notImplemented, "cop1"},
    {18, cop2, "cop2"},
    {19, notImplemented, "cop3"},
    {20, invalid, "INVALID"},
    {21, invalid, "INVALID"},
    {22, invalid, "INVALID"},
    {23, invalid, "INVALID"},

    {24, invalid, "INVALID"},
    {25, invalid, "INVALID"},
    {26, invalid, "INVALID"},
    {27, invalid, "INVALID"},
    {28, invalid, "INVALID"},
    {29, invalid, "INVALID"},
    {30, invalid, "INVALID"},
    {31, invalid, "INVALID"},

    {32, lb, "lb"},
    {33, lh, "lh"},
    {34, lwl, "lwl"},
    {35, lw, "lw"},
    {36, lbu, "lbu"},
    {37, lhu, "lhu"},
    {38, lwr, "lwr"},
    {39, invalid, "INVALID"},

    {40, sb, "sb"},
    {41, sh, "sh"},
    {42, swl, "swl"},
    {43, sw, "sw"},
    {44, invalid, "INVALID"},
    {45, invalid, "INVALID"},
    {46, swr, "swr"},
    {47, invalid, "INVALID"},

    {48, notImplemented, "lwc0"},
    {49, notImplemented, "lwc1"},
    {50, dummy, "lwc2"},
    {51, notImplemented, "lwc3"},
    {52, invalid, "INVALID"},
    {53, invalid, "INVALID"},
    {54, invalid, "INVALID"},
    {55, invalid, "INVALID"},

    {56, notImplemented, "swc0"},
    {57, notImplemented, "swc1"},
    {58, dummy, "swc2"},
    {59, notImplemented, "swc3"},
    {60, invalid, "INVALID"},
    {61, invalid, "INVALID"},
    {62, invalid, "INVALID"},
    {63, invalid, "INVALID"},
    //{63, breakpoint, "BREAKPOINT"}
};

PrimaryInstruction SpecialTable[64] = {
    {0, sll, "sll"},
    {1, invalid, "INVALID"},
    {2, srl, "srl"},
    {3, sra, "sra"},
    {4, sllv, "sllv"},
    {5, invalid, "INVALID"},
    {6, srlv, "srlv"},
    {7, srav, "srav"},

    {8, jr, "jr"},
    {9, jalr, "jalr"},
    {10, invalid, "INVALID"},
    {11, invalid, "INVALID"},
    {12, syscall, "syscall"},
    {13, break_, "break"},
    {14, invalid, "INVALID"},
    {15, invalid, "INVALID"},

    {16, mfhi, "mfhi"},
    {17, mthi, "mthi"},
    {18, mflo, "mflo"},
    {19, mtlo, "mtlo"},
    {20, invalid, "INVALID"},
    {21, invalid, "INVALID"},
    {22, invalid, "INVALID"},
    {23, invalid, "INVALID"},

    {24, mult, "mult"},
    {25, multu, "multu"},
    {26, div, "div"},
    {27, divu, "divu"},
    {28, invalid, "INVALID"},
    {29, invalid, "INVALID"},
    {30, invalid, "INVALID"},
    {31, invalid, "INVALID"},

    {32, add, "add"},
    {33, addu, "addu"},
    {34, sub, "sub"},
    {35, subu, "subu"},
    {36, and, "and"},
    {37, or, "or"},
    {38, xor, "xor"},
    {39, nor, "nor"},

    {40, invalid, "INVALID"},
    {41, invalid, "INVALID"},
    {42, slt, "slt"},
    {43, sltu, "sltu"},
    {44, invalid, "INVALID"},
    {45, invalid, "INVALID"},
    {46, invalid, "INVALID"},
    {47, invalid, "INVALID"},

    {48, invalid, "INVALID"},
    {49, invalid, "INVALID"},
    {50, invalid, "INVALID"},
    {51, invalid, "INVALID"},
    {52, invalid, "INVALID"},
    {53, invalid, "INVALID"},
    {54, invalid, "INVALID"},
    {55, invalid, "INVALID"},

    {56, invalid, "INVALID"},
    {57, invalid, "INVALID"},
    {58, invalid, "INVALID"},
    {59, invalid, "INVALID"},
    {60, invalid, "INVALID"},
    {61, invalid, "INVALID"},
    {62, invalid, "INVALID"},
    {63, invalid, "INVALID"},
};

int part = 0;

inline bool isOverflow(uint32_t result, uint32_t a, uint32_t b) {
#define SIGN(x) ((x)&0x80000000)
    return !(SIGN(a) ^ SIGN(b)) && (SIGN(result) ^ SIGN(a));
#undef SIGN
}

void exception(mips::CPU *cpu, cop0::CAUSE::Exception cause) {
    cpu->cop0.cause.exception = cause;

    if (cpu->shouldJump) {
        cpu->cop0.cause.isInDelaySlot = true;
        cpu->cop0.epc = cpu->PC - 4;  // EPC - return address from trap
    } else {
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
void sll(CPU *cpu, Opcode i) {
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
void srl(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, %d", i.rd, i.rt, i.sh);
    cpu->reg[i.rd] = cpu->reg[i.rt] >> i.sh;
}

// Shift Word Right Arithmetic
// SRA rd, rt, a
void sra(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, %d", i.rd, i.rt, i.sh);
    cpu->reg[i.rd] = ((int32_t)cpu->reg[i.rt]) >> i.sh;
}

// Shift Word Left Logical Variable
// SLLV rd, rt, rs
void sllv(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rt, i.rs);
    cpu->reg[i.rd] = cpu->reg[i.rt] << (cpu->reg[i.rs] & 0x1f);
}

// Shift Word Right Logical Variable
// SRLV rd, rt, a
void srlv(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, %d", i.rd, i.rt, i.sh);
    cpu->reg[i.rd] = cpu->reg[i.rt] >> (cpu->reg[i.rs] & 0x1f);
}

// Shift Word Right Arithmetic Variable
// SRAV rd, rt, rs
void srav(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rt, i.rs);
    cpu->reg[i.rd] = ((int32_t)cpu->reg[i.rt]) >> (cpu->reg[i.rs] & 0x1f);
}

// Jump Register
// JR rs
void jr(CPU *cpu, Opcode i) {
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
void jalr(CPU *cpu, Opcode i) {
    disasm("r%d r%d", i.rd, i.rs);
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
void syscall(CPU *cpu, Opcode i) { exception(cpu, cop0::CAUSE::Exception::syscall); }

// Break
// BREAK
void break_(CPU *cpu, Opcode i) { exception(cpu, cop0::CAUSE::Exception::breakpoint); }

// Move From Hi
// MFHI rd
void mfhi(CPU *cpu, Opcode i) {
    disasm("r%d", i.rd);
    cpu->reg[i.rd] = cpu->hi;
}

// Move To Hi
// MTHI rd
void mthi(CPU *cpu, Opcode i) {
    disasm("r%d", i.rs);
    cpu->hi = cpu->reg[i.rs];
}

// Move From Lo
// MFLO rd
void mflo(CPU *cpu, Opcode i) {
    disasm("r%d", i.rd);
    cpu->reg[i.rd] = cpu->lo;
}

// Move To Lo
// MTLO rd
void mtlo(CPU *cpu, Opcode i) {
    disasm("r%d", i.rs);
    cpu->lo = cpu->reg[i.rs];
}

// Multiply
// mult rs, rt
void mult(CPU *cpu, Opcode i) {
    disasm("r%d, r%d", i.rs, i.rt);
    uint64_t temp = (int64_t)(int32_t)cpu->reg[i.rs] * (int64_t)(int32_t)cpu->reg[i.rt];
    cpu->lo = temp & 0xffffffff;
    cpu->hi = temp >> 32;
}

// Multiply Unsigned
// multu rs, rt
void multu(CPU *cpu, Opcode i) {
    disasm("r%d, r%d", i.rs, i.rt);
    uint64_t temp = (uint64_t)cpu->reg[i.rs] * (uint64_t)cpu->reg[i.rt];
    cpu->lo = temp & 0xffffffff;
    cpu->hi = temp >> 32;
}
// Divide
// div rs, rt
void div(CPU *cpu, Opcode i) {
    disasm("r%d, r%d", i.rs, i.rt);

    int32_t rs = (int32_t)cpu->reg[i.rs];
    int32_t rt = (int32_t)cpu->reg[i.rt];
    if (rt == 0) {
        cpu->lo = (rs < 0) ? 0x00000001 : 0xffffffff;
        cpu->hi = rs;
    } else if (rs == 0x80000000 && rt == 0xffffffff) {
        cpu->lo = 0x80000000;
        cpu->hi = 0x00000000;
    } else {
        cpu->lo = rs / rt;
        cpu->hi = rs % rt;
    }
}

// Divide Unsigned Word
// divu rs, rt
void divu(CPU *cpu, Opcode i) {
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
void add(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);

    uint32_t a = cpu->reg[i.rs];
    uint32_t b = cpu->reg[i.rt];
    uint32_t result = a + b;

    if (isOverflow(result, a, b)) {
        exception(cpu, cop0::CAUSE::Exception::arithmeticOverflow);
        return;
    }
    cpu->reg[i.rd] = result;
}

// Add unsigned
// addu rd, rs, rt
void addu(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
    cpu->reg[i.rd] = cpu->reg[i.rs] + cpu->reg[i.rt];
}

// Subtract
// sub rd, rs, rt
void sub(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
    uint32_t a = cpu->reg[i.rs];
    uint32_t b = cpu->reg[i.rt];
    uint32_t result = a - b;

    if (isOverflow(result, a, b)) {
        exception(cpu, cop0::CAUSE::Exception::arithmeticOverflow);
        return;
    }
    cpu->reg[i.rd] = result;
}

// Subtract unsigned
// subu rd, rs, rt
void subu(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
    cpu->reg[i.rd] = cpu->reg[i.rs] - cpu->reg[i.rt];
}

// And
// and rd, rs, rt
void and (CPU * cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
    cpu->reg[i.rd] = cpu->reg[i.rs] & cpu->reg[i.rt];
}

// Or
// OR rd, rs, rt
void or (CPU * cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
    cpu->reg[i.rd] = cpu->reg[i.rs] | cpu->reg[i.rt];
}

// Xor
// XOR rd, rs, rt
void xor
    (CPU * cpu, Opcode i) {
        disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
        cpu->reg[i.rd] = cpu->reg[i.rs] ^ cpu->reg[i.rt];
    }

    // Nor
    // NOR rd, rs, rt
    void nor(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
    cpu->reg[i.rd] = ~(cpu->reg[i.rs] | cpu->reg[i.rt]);
}

// Set On Less Than Signed
// SLT rd, rs, rt
void slt(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, r%d", i.rd, i.rs, i.rt);
    if ((int32_t)cpu->reg[i.rs] < (int32_t)cpu->reg[i.rt])
        cpu->reg[i.rd] = 1;
    else
        cpu->reg[i.rd] = 0;
}

// Set On Less Than Unsigned
// SLTU rd, rs, rt
void sltu(CPU *cpu, Opcode i) {
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
            mnemonic("BLTZ");
            disasm("r%d, %d", i.rs, i.offset);

            if ((int32_t)cpu->reg[i.rs] < 0) {
                cpu->shouldJump = true;
                cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset << 2);
            }
            break;

        case 1:
            // Branch On Greater Than Or Equal To Zero
            // BGEZ rs, offset
            mnemonic("BGEZ");
            disasm("r%d, %d", i.rs, i.offset);

            if ((int32_t)cpu->reg[i.rs] >= 0) {
                cpu->shouldJump = true;
                cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset << 2);
            }
            break;

        case 16:
            // Branch On Less Than Zero And Link
            // bltzal rs, offset
            mnemonic("BLTZAL");
            disasm("r%d, %d", i.rs, i.offset);

            cpu->reg[31] = cpu->PC + 8;
            if ((int32_t)cpu->reg[i.rs] < 0) {
                cpu->shouldJump = true;
                cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset << 2);
            }
            break;

        case 17:
            // Branch On Greater Than Or Equal To Zero And Link
            // BGEZAL rs, offset
            mnemonic("BGEZAL");
            disasm("r%d, %d", i.rs, i.offset);

            cpu->reg[31] = cpu->PC + 8;
            if ((int32_t)cpu->reg[i.rs] >= 0) {
                cpu->shouldJump = true;
                cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset << 2);
            }
            break;

        default:
            invalid(cpu, i);
    }
}

// Jump
// J target
void j(CPU *cpu, Opcode i) {
    disasm("0x%x", i.target << 2);
    cpu->shouldJump = true;
    cpu->jumpPC = (cpu->PC & 0xf0000000) | (i.target << 2);
}

// Jump And Link
// JAL target
void jal(CPU *cpu, Opcode i) {
    disasm("0x%x", (i.target << 2));
    cpu->shouldJump = true;
    cpu->jumpPC = (cpu->PC & 0xf0000000) | (i.target << 2);
    cpu->reg[31] = cpu->PC + 8;
}

// Branch On Equal
// BEQ rs, rt, offset
void beq(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, %d", i.rs, i.rt, i.offset);
    if (cpu->reg[i.rt] == cpu->reg[i.rs]) {
        cpu->shouldJump = true;
        cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset << 2);
    }
}

// Branch On Greater Than Zero
// BGTZ rs, offset
void bgtz(CPU *cpu, Opcode i) {
    disasm("r%d, %d", i.rs, i.offset);
    if ((int32_t)cpu->reg[i.rs] > 0) {
        cpu->shouldJump = true;
        cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset << 2);
    }
}

// Branch On Less Than Or Equal To Zero
// BLEZ rs, offset
void blez(CPU *cpu, Opcode i) {
    disasm("r%d, %d", i.rs, i.offset);
    if ((int32_t)cpu->reg[i.rs] <= 0) {
        cpu->shouldJump = true;
        cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset << 2);
    }
}

// Branch On Not Equal
// BNE rs, offset
void bne(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, %d", i.rs, i.rt, i.offset);
    if (cpu->reg[i.rt] != cpu->reg[i.rs]) {
        cpu->shouldJump = true;
        cpu->jumpPC = (int32_t)(cpu->PC + 4) + (i.offset << 2);
    }
}

// Add Immediate Word
// ADDI rt, rs, imm
void addi(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, %d", i.rt, i.rs, i.offset);
    uint32_t a = cpu->reg[i.rs];
    uint32_t b = i.offset;
    uint32_t result = a + b;

    if (isOverflow(result, a, b)) {
        exception(cpu, cop0::CAUSE::Exception::arithmeticOverflow);
        return;
    }
    cpu->reg[i.rt] = result;
}

// Add Immediate Unsigned Word
// ADDIU rt, rs, imm
void addiu(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, %d", i.rt, i.rs, i.offset);
    cpu->reg[i.rt] = cpu->reg[i.rs] + i.offset;
}

// Set On Less Than Immediate
// SLTI rd, rs, rt
void slti(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, 0x%x", i.rt, i.rs, i.imm);
    if ((int32_t)cpu->reg[i.rs] < (int32_t)i.offset)
        cpu->reg[i.rt] = 1;
    else
        cpu->reg[i.rt] = 0;
}

// Set On Less Than Immediate Unsigned
// SLTIU rd, rs, rt
void sltiu(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, 0x%x", i.rt, i.rs, i.imm);
    if (cpu->reg[i.rs] < (uint32_t)i.offset)
        cpu->reg[i.rt] = 1;
    else
        cpu->reg[i.rt] = 0;
}

// And Immediate
// ANDI rt, rs, imm
void andi(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, 0x%x", i.rt, i.rs, i.imm);
    cpu->reg[i.rt] = cpu->reg[i.rs] & i.imm;
}

// Or Immediete
// ORI rt, rs, imm
void ori(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, 0x%x", i.rt, i.rs, i.imm);
    cpu->reg[i.rt] = cpu->reg[i.rs] | i.imm;
}

// Xor Immediate
// XORI rt, rs, imm
void xori(CPU *cpu, Opcode i) {
    disasm("r%d, r%d, 0x%x", i.rt, i.rs, i.imm);
    cpu->reg[i.rt] = cpu->reg[i.rs] ^ i.imm;
}

// Load Upper Immediate
// LUI rt, imm
void lui(CPU *cpu, Opcode i) {
    disasm("r%d, 0x%x", i.rt, i.imm);
    cpu->reg[i.rt] = i.imm << 16;
}

// Coprocessor zero
void cop0(CPU *cpu, Opcode i) {
    switch (i.rs) {
        case 0:
            // Move from co-processor zero
            // MFC0 rd, <nn>
            mnemonic("MFC0");
            disasm("r%d, $%d", i.rt, i.rd);

            switch (i.rd) {
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
                case 12:
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
void cop2(CPU *cpu, Opcode i) { printf("COP2: 0x%08x\n", i.opcode); }

// Load Byte
// LB rt, offset(base)
void lb(CPU *cpu, Opcode i) {
    disasm("r%d, %d(r%d)", i.rt, i.offset, i.rs);
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    cpu->reg[i.rt] = ((int32_t)(cpu->readMemory8(addr) << 24)) >> 24;
}

// Load Halfword
// LH rt, offset(base)
void lh(CPU *cpu, Opcode i) {
    disasm("r%d, %d(r%d)", i.rt, i.offset, i.rs);
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    if (addr & 1)  // non aligned address
    {
        exception(cpu, cop0::CAUSE::Exception::addressErrorLoad);
        return;
    }
    cpu->reg[i.rt] = (int32_t)(int16_t)cpu->readMemory16(addr);
}

// Load Word Left
// LWL rt, offset(base)
void lwl(CPU *cpu, Opcode i) {
    disasm("r%d, %d(r%d)", i.rt, i.offset, i.rs);

    uint32_t addr = cpu->reg[i.rs] + i.offset;

    uint32_t rt = cpu->reg[i.rt];
    uint32_t word = cpu->readMemory32(addr & 0xfffffffc);

    uint32_t result = 0;
    switch (i.offset % 4) {
        case 0:
            result = (word << 24) | (rt & 0xffffff);
            break;
        case 1:
            result = (word << 16) | (rt & 0xffff);
            break;
        case 2:
            result = (word << 8) | (rt & 0xff);
            break;
        case 3:
            result = word;
            break;
    }
    cpu->reg[i.rt] = result;
}

// Load Word
// LW rt, offset(base)
void lw(CPU *cpu, Opcode i) {
    disasm("r%d, %d(r%d)", i.rt, i.offset, i.rs);
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    if (addr & 3)  // non aligned address
    {
        exception(cpu, cop0::CAUSE::Exception::addressErrorLoad);
        return;
    }
    cpu->reg[i.rt] = cpu->readMemory32(addr);
}

// Load Byte Unsigned
// LBU rt, offset(base)
void lbu(CPU *cpu, Opcode i) {
    disasm("r%d, %d(r%d)", i.rt, i.offset, i.rs);
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    cpu->reg[i.rt] = cpu->readMemory8(addr);
}

// Load Halfword Unsigned
// LHU rt, offset(base)
void lhu(CPU *cpu, Opcode i) {
    disasm("r%d, %d(r%d)", i.rt, i.offset, i.rs);
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    if (addr & 1)  // non aligned address
    {
        exception(cpu, cop0::CAUSE::Exception::addressErrorLoad);
        return;
    }
    cpu->reg[i.rt] = cpu->readMemory16(addr);
}

// Load Word Right
// LWR rt, offset(base)
void lwr(CPU *cpu, Opcode i) {
    disasm("r%d, %d(r%d)", i.rt, i.offset, i.rs);

    uint32_t addr = cpu->reg[i.rs] + i.offset;
    uint32_t rt = cpu->reg[i.rt];
    uint32_t word = cpu->readMemory32(addr & 0xfffffffc);

    uint32_t result = 0;

    switch (i.offset % 4) {
        case 0:
            result = word;
            break;
        case 1:
            result = (rt & 0xff000000) | (word >> 8);
            break;
        case 2:
            result = (rt & 0xffff0000) | (word >> 16);
            break;
        case 3:
            result = (rt & 0xffffff00) | (word >> 24);
            break;
    }
    cpu->reg[i.rt] = result;
}

// Store Byte
// SB rt, offset(base)
void sb(CPU *cpu, Opcode i) {
    disasm("r%d, %d(r%d)", i.rt, i.offset, i.rs);
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    cpu->writeMemory8(addr, cpu->reg[i.rt]);
}

// Store Halfword
// SH rt, offset(base)
void sh(CPU *cpu, Opcode i) {
    disasm("r%d, %d(r%d)", i.rt, i.offset, i.rs);
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
void swl(CPU *cpu, Opcode i) {
    disasm("r%d, %d(r%d)", i.rt, i.offset, i.rs);

    uint32_t addr = cpu->reg[i.rs] + i.offset;
    uint32_t rt = cpu->reg[i.rt];
    uint32_t word = cpu->readMemory32(addr & 0xfffffffc);

    uint32_t result = 0;
    switch (i.offset % 4) {
        case 0:
            result = (word & 0xffffff00) | ((rt >> 24) & 0xff);
            break;
        case 1:
            result = (word & 0xffff0000) | ((rt >> 16) & 0xffff);
            break;
        case 2:
            result = (word & 0xff000000) | ((rt >> 8) & 0xffffff);
            break;
        case 3:
            result = rt;
            break;
    }
    cpu->writeMemory32(addr & 0xfffffffc, result);
}

// Store Word
// SW rt, offset(base)
void sw(CPU *cpu, Opcode i) {
    disasm("r%d, %d(r%d)", i.rt, i.offset, i.rs);
    uint32_t addr = cpu->reg[i.rs] + i.offset;
    if (addr & 3)  // non aligned address
    {
        exception(cpu, cop0::CAUSE::Exception::addressErrorStore);
        return;
    }
    cpu->writeMemory32(addr, cpu->reg[i.rt]);
}

// Store Word Right
// SWR rt, offset(base)
void swr(CPU *cpu, Opcode i) {
    disasm("r%d, %d(r%d)", i.rt, i.offset, i.rs);

    uint32_t addr = cpu->reg[i.rs] + i.offset;
    uint32_t rt = cpu->reg[i.rt];
    uint32_t word = cpu->readMemory32(addr & 0xfffffffc);

    uint32_t result = 0;

    switch (i.offset % 4) {
        case 0:
            result = rt;
            break;
        case 1:
            result = ((rt << 8) & 0xffffff00) | (word & 0xff);
            break;
        case 2:
            result = ((rt << 16) & 0xffff0000) | (word & 0xffff);
            break;
        case 3:
            result = ((rt << 24) & 0xff000000) | (word & 0xffffff);
            break;
    }
    cpu->writeMemory32(addr & 0xfffffffc, result);
}

// BREAKPOINT
void breakpoint(CPU *cpu, Opcode i) {
    disasm("");
    printf("Breakpoint at 0x%08x\n", cpu->PC);
    cpu->state = CPU::State::halted;
}
};
