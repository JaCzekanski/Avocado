#pragma once
#include <array>
#include <string>

struct System;

namespace gui::options {
class MemoryCard {
    bool loadPaths = true;
    std::array<std::string, 2> cardPaths;
    void memoryCardWindow(System* sys);

   public:
    bool memoryCardWindowOpen = false;
    void displayWindows(System* sys);
};
}  // namespace gui::options::memory_card