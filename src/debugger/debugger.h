#pragma once
#include "mipsInstructions.h"

namespace debugger {
struct Instruction {
    std::string mnemonic;
    std::string parameters;
};
extern bool mapRegisterNames;
std::string reg(int n);
Instruction decodeInstruction(mipsInstructions::Opcode &i);
};
