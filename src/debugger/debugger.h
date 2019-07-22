#pragma once
#include <string>
#include "cpu/opcode.h"

namespace debugger {
struct Instruction {
    mips::Opcode opcode;
    std::string mnemonic;
    std::string parameters;
    bool valid = true;

    bool isBranch() const;
};

extern bool mapRegisterNames;
extern bool followPC;
std::string reg(int n);
Instruction decodeInstruction(mips::Opcode& i);
};  // namespace debugger
