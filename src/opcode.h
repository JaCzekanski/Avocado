#pragma once
#include <cstdint>

namespace mips {
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

    Opcode(uint32_t v) : opcode(v) {}
};
};