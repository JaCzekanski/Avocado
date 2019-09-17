#pragma once
#include <memory>
#include <string>

struct System;

namespace system_tools {

void bootstrap(std::unique_ptr<System>& sys);
void loadFile(std::unique_ptr<System>& sys, std::string path);
void saveMemoryCards(std::unique_ptr<System>& sys, bool force = false);
std::unique_ptr<System> hardReset();

};  // namespace system_tools