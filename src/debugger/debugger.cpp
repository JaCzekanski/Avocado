#include "debugger.h"
#include <fmt/core.h>
#include <array>

namespace debugger {
bool mapRegisterNames = true;
bool followPC = true;

// clang-format off
	std::array<const char*, 32> regNames = {
		"zero",
		"at",
		"v0", "v1",
		"a0", "a1", "a2", "a3",
		"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
		"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
		"t8", "t9",
		"k0", "k1",
		"gp",
		"sp",
		"fp",
		"ra",
	};

    std::array<const char*, 16> cop0RegNames = {
        "cop0r0?",   // r0
        "cop0r1?",   // r1
        "cop0r2?",   // r2
        "bpc",       // r3
        "cop0r4?",   // r4
        "bda",       // r5
        "tar",       // r6
        "dcic",      // r7
        "bada",      // r8
        "bdam",      // r9
        "cop0r10?",  // r10
        "bpcm",      // r11
        "status",    // r12
        "cause",     // r13
        "epc",       // r14
        "prid",      // r15
    };

    std::array<const char*, 64> cop2RegNames = {
        // Data
        "vxy0",      // r0
        "vz0",       // r1
        "vxy1",      // r2
        "vz1",       // r3
        "vxy2",      // r4
        "vz2",       // r5
        "rgbc",      // r6
        "otz",       // r7
        "ir0",       // r8
        "ir1",       // r9
        "ir2",       // r10
        "ir3",       // r11
        "sxy0",      // r12
        "sxy1",      // r13
        "sxy2",      // r14
        "sxyp",      // r15
        "sz0",       // r16
        "sz1",       // r17
        "sz2",       // r18
        "sz3",       // r19
        "rgb0",      // r20
        "rgb1",      // r21
        "rgb2",      // r22
        "res1",      // r23
        "mac0",      // r24
        "mac1",      // r25
        "mac2",      // r26
        "mac3",      // r27
        "irgb",      // r28
        "orgb",      // r29
        "lzcs",      // r30
        "lzcr",      // r31
        // Control
        "rt11-rt12", // r32
        "rt13-rt21", // r33
        "rt22-rt23", // r34
        "rt31-rt32", // r35
        "rt33",      // r36
        "trx",       // r37
        "try",       // r38
        "trz",       // r39
        "l11-l12",   // r40
        "l13-l21",   // r41
        "l22-l23",   // r42
        "l31-l32",   // r43
        "l33",       // r44
        "rbk",       // r45
        "gbk",       // r46
        "bbk",       // r47
        "lr1-lr2",   // r48
        "lr3-lg1",   // r49
        "lg2-lg3",   // r50
        "lb1-lb2",   // r51
        "lb3",       // r52
        "rfc",       // r53
        "gfc",       // r54
        "bfc",       // r55
        "ofx",       // r56
        "ofy",       // r57
        "h",         // r58
        "dqa",       // r59
        "dqb",       // r60
        "zsf3",      // r61
        "zsf4",      // r62
        "flag",      // r63
    };
// clang-format on

bool Instruction::isBranch() const {
    auto sub = (opcode.rt & 0x11);
    if (opcode.op == 1 && (sub == 0 || sub == 1 || sub == 16 || sub == 17)) return true;
    if (opcode.op == 4 || opcode.op == 5 || opcode.op == 6 || opcode.op == 7) return true;
    return false;
}

std::string reg(unsigned int n) {
    if (mapRegisterNames) return regNames[n];
    return fmt::format("r{}", n);
}

std::string cop0reg(unsigned int n) {
    if (mapRegisterNames && n < cop0RegNames.size()) return cop0RegNames.at(n);
    return fmt::format("cop0r{}", n);
}

std::string cop2reg(unsigned int n) {
    if (mapRegisterNames && n < cop2RegNames.size()) return cop2RegNames.at(n);
    return fmt::format("cop2r{}", n);
}

Instruction mapSpecialInstruction(mips::Opcode& i);

Instruction decodeInstruction(mips::Opcode& i) {
    Instruction ins;
    ins.opcode = i;

#define U16(x) static_cast<unsigned short>(x)

#define R(x) reg(x).c_str()
#define LOADTYPE fmt::format("{}, 0x{:04x}({})", R(i.rt), U16(i.offset), R(i.rs));
#define ITYPE fmt::format("{}, {}, 0x{:04x}", R(i.rt), R(i.rs), U16(i.offset))
#define JTYPE fmt::format("0x{:x}", i.target * 4)
#define O(n, m, d)                     \
    case n:                            \
        ins.mnemonic = std::string(m); \
        ins.parameters = d;            \
        break

#define BRANCH_TYPE fmt::format("{}, {}", R(i.rs), U16(i.offset))

    switch (i.op) {
        case 0: ins = mapSpecialInstruction(i); break;

        case 1:
            switch (i.rt & 0x11) {
                O(0, "bltz", BRANCH_TYPE);
                O(1, "bgez", BRANCH_TYPE);
                O(16, "bltzal", BRANCH_TYPE);
                O(17, "bgezal", BRANCH_TYPE);

                default:
                    ins.mnemonic = fmt::format("0x{:08x}", i.opcode);
                    ins.valid = false;
                    break;
            }
            break;

            O(2, "j", JTYPE);
            O(3, "jal", JTYPE);
            O(4, "beq", ITYPE);
            O(5, "bne", ITYPE);
            O(6, "blez", BRANCH_TYPE);
            O(7, "bgtz", BRANCH_TYPE);

            O(8, "addi", ITYPE);
            O(9, "addiu", ITYPE);
            O(10, "slti", ITYPE);
            O(11, "sltiu", ITYPE);
            O(12, "andi", ITYPE);
            O(13, "ori", ITYPE);
            O(14, "xori", ITYPE);
            O(15, "lui", fmt::format("{}, 0x{:04x}", R(i.rt), U16(i.offset)));

        case 16:
            switch (i.rs) {
                case 0:
                    ins.mnemonic = std::string("mfc0");
                    ins.parameters = fmt::format("{}, {}", R(i.rt), cop0reg(i.rd).c_str());
                    ins.valid = i.rd < 16;
                    break;
                case 4:
                    ins.mnemonic = std::string("mtc0");
                    ins.parameters = fmt::format("{}, {}", R(i.rt), cop0reg(i.rd).c_str());
                    ins.valid = i.rd < 16;
                    break;
                    O(16, "rfe", "");
                default:
                    ins.mnemonic = fmt::format("cop0 - 0x{:08x}", i.opcode);
                    ins.valid = false;
                    break;
            }
            break;
            O(17, "cop1", "");

        case 18:
            switch (i.rs) {
                O(0, "mfc2", fmt::format("{}, {}", R(i.rt), cop2reg(i.rd).c_str()));
                O(2, "cfc2", fmt::format("{}, {}", R(i.rt), cop2reg(i.rd + 32).c_str()));
                O(4, "mtc2", fmt::format("{}, {}", R(i.rt), cop2reg(i.rd).c_str()));
                O(6, "ctc2", fmt::format("{}, {}", R(i.rt), cop2reg(i.rd + 32).c_str()));
                default:
                    ins.mnemonic = fmt::format("cop2 - 0x{:08x}", i.opcode);
                    ins.valid = false;
                    break;
            }
            break;
            O(19, "cop3", "");

            O(32, "lb", LOADTYPE);
            O(33, "lh", LOADTYPE);
            O(34, "lwl", LOADTYPE);
            O(35, "lw", LOADTYPE);
            O(36, "lbu", LOADTYPE);
            O(37, "lhu", LOADTYPE);
            O(38, "lwr", LOADTYPE);

            O(40, "sb", LOADTYPE);
            O(41, "sh", LOADTYPE);
            O(42, "swl", LOADTYPE);
            O(43, "sw", LOADTYPE);
            O(46, "swr", LOADTYPE);

            // TODO: Add dummy instructions
            O(50, "lwc2", "");  // TODO: add valid prototype
            O(58, "swc2", "");  // TODO: add valid prototype

        default:
            ins.mnemonic = fmt::format("0x{:08x}", i.opcode);
            ins.valid = false;
            break;
    }
    return ins;
}

Instruction mapSpecialInstruction(mips::Opcode& i) {
    Instruction ins;
    ins.opcode = i;
#define SHIFT_TYPE fmt::format("{}, {}, {}", R(i.rd), R(i.rt), (int)i.sh)
#define ATYPE fmt::format("{}, {}, {}", R(i.rd), R(i.rs), R(i.rt))

    switch (i.fun) {
        case 0:
            if (i.rt == 0 && i.rd == 0 && i.sh == 0) {
                ins.mnemonic = "nop";
            } else {
                ins.mnemonic = "sll";
                ins.parameters = SHIFT_TYPE;
            }
            break;

            O(2, "srl", SHIFT_TYPE);
            O(3, "sra", SHIFT_TYPE);
            O(4, "sllv", SHIFT_TYPE);
            O(6, "srlv", SHIFT_TYPE);
            O(7, "srav", SHIFT_TYPE);

            O(8, "jr", fmt::format("{}", R(i.rs)));
            O(9, "jalr", fmt::format("{}, {}", R(i.rd), R(i.rs)));
            O(12, "syscall", "");
            O(13, "break", "");

            O(16, "mfhi", fmt::format("{}", R(i.rd)));
            O(17, "mthi", fmt::format("{}", R(i.rs)));
            O(18, "mflo", fmt::format("{}", R(i.rd)));
            O(19, "mtlo", fmt::format("{}", R(i.rs)));

            O(24, "mult", fmt::format("{}, {}", R(i.rs), R(i.rt)));
            O(25, "multu", fmt::format("{}, {}", R(i.rs), R(i.rt)));
            O(26, "div", fmt::format("{}, {}", R(i.rs), R(i.rt)));
            O(27, "divu", fmt::format("{}, {}", R(i.rs), R(i.rt)));

            O(32, "add", ATYPE);
            O(33, "addu", ATYPE);
            O(34, "sub", ATYPE);
            O(35, "subu", ATYPE);
            O(36, "and", ATYPE);
            O(37, "or", ATYPE);
            O(38, "xor", ATYPE);
            O(39, "nor", ATYPE);
            O(42, "slt", ATYPE);
            O(43, "sltu", ATYPE);

        default:
            ins.mnemonic = fmt::format("special 0x{:02x}", (int)i.fun);
            ins.valid = false;
            break;
    }
    return ins;
}
};  // namespace debugger
