#include "debugger.h"
#include "utils/string.h"

namespace debugger {
bool mapRegisterNames = true;

// clang-format off
	const char* regNames[] = {
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
		"ra"
	};
// clang-format on

std::string reg(int n) {
    if (mapRegisterNames) return regNames[n];
    return string_format("r%d", n);
}

Instruction mapSpecialInstruction(mips::Opcode& i);

Instruction decodeInstruction(mips::Opcode& i) {
    Instruction ins;

#define R(x) reg(x).c_str()
#define LOADTYPE string_format("%s, 0x%hx(%s)", R(i.rt), i.offset, R(i.rs));
#define ITYPE string_format("%s, %s, 0x%hx", R(i.rt), R(i.rs), i.offset)
#define JTYPE string_format("0x%x", i.target * 4)
#define O(n, m, d)                     \
    case n:                            \
        ins.mnemonic = std::string(m); \
        ins.parameters = d;            \
        break

#define BRANCH_TYPE string_format("%s, %d", R(i.rs), i.offset)

    switch (i.op) {
        case 0:
            ins = mapSpecialInstruction(i);
            break;

        case 1:
            switch (i.rt & 0x11) {
                O(0, "bltz", BRANCH_TYPE);
                O(1, "bgez", BRANCH_TYPE);
                O(16, "bltzal", BRANCH_TYPE);
                O(17, "bgezal", BRANCH_TYPE);

                default:
                    ins.mnemonic = string_format("0x%08x", i.opcode);
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
            O(15, "lui", ITYPE);

            O(16, "cop0", "");
            O(17, "cop1", "");
            O(18, "cop2", "");
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

            O(50, "lwc2", "");
            O(58, "swc2", "");

        default:
            ins.mnemonic = string_format("0x%08x", i.opcode);
            break;
    }
    return ins;
}

Instruction mapSpecialInstruction(mips::Opcode& i) {
    Instruction ins;
#define SHIFT_TYPE string_format("%s, %s, %d", R(i.rd), R(i.rt), i.sh)
#define ATYPE string_format("%s, %s, %s", R(i.rd), R(i.rs), R(i.rt))

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

            O(8, "jr", string_format("%s", R(i.rs)));
            O(9, "jalr", string_format("%s %s", R(i.rd), R(i.rs)));
            O(12, "syscall", "");
            O(13, "break", "");

            O(16, "mfhi", string_format("%s", R(i.rd)));
            O(17, "mthi", string_format("%s", R(i.rs)));
            O(18, "mflo", string_format("%s", R(i.rd)));
            O(19, "mtlo", string_format("%s", R(i.rs)));

            O(24, "mult", string_format("%s, %s", R(i.rs), R(i.rt)));
            O(25, "multu", string_format("%s, %s", R(i.rs), R(i.rt)));
            O(26, "div", string_format("%s, %s", R(i.rs), R(i.rt)));
            O(27, "divu", string_format("%s, %s", R(i.rs), R(i.rt)));

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
            ins.mnemonic = string_format("SPECIAL 0x%02x", i.fun);
            break;
    }
    return ins;
}
};  // namespace debugger
