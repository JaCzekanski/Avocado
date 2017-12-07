#pragma once
#include <string>
#include "cpu/opcode.h"

namespace debugger {
struct Instruction {
    std::string mnemonic;
    std::string parameters;
};

extern bool mapRegisterNames;
std::string reg(int n);
Instruction decodeInstruction(mips::Opcode& i);
};
