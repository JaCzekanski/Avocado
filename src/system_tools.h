#pragma once
#include <memory>
#include <string>

struct System;

namespace system_tools {

void bootstrap(std::unique_ptr<System>& sys);
void loadFile(std::unique_ptr<System>& sys, const std::string& path);
bool loadMemoryCard(std::unique_ptr<System>& sys, int slot);
bool saveMemoryCard(std::unique_ptr<System>& sys, int slot, bool force = false);
std::unique_ptr<System> hardReset();

};  // namespace system_tools