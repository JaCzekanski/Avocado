#pragma once
#include "mipsInstructions.h"

namespace debugger {
extern bool mapRegisterNames;
std::string decodeInstruction(mipsInstructions::Opcode &i);
};
