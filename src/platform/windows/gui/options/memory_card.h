#pragma once
#include <array>
#include <string>

struct System;
namespace peripherals {
struct MemoryCard;
};

namespace gui::options {
class MemoryCard {
    bool loadPaths = true;
    std::array<std::string, 2> cardPaths;

    void parseAndDisplayCard(peripherals::MemoryCard* card);
    void memoryCardWindow(System* sys);

   public:
    bool memoryCardWindowOpen = false;
    void displayWindows(System* sys);
};
}  // namespace gui::options