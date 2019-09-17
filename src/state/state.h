#pragma once
#include <string>

struct System;

namespace state {
using SaveState = std::string;

SaveState save(System* sys);
bool load(System* sys, const SaveState& state);

bool saveToFile(System* sys, const std::string& path);
bool loadFromFile(System* sys, const std::string& path);

void quickSave(System* sys, int slot = 0);
void quickLoad(System* sys, int slot = 0);

bool saveLastState(System* sys);
bool loadLastState(System* sys);

// Time travel / rewind / state history
void manageTimeTravel(System* sys);
bool rewindState(System* sys);
};  // namespace state