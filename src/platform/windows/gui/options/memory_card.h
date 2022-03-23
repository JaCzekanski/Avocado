#pragma once
#include <array>
#include <string>

struct System;
namespace peripherals {
struct MemoryCard;
};

namespace gui::options {

enum class Region { Unknown, Japan, Europe, America };

class MemoryCard {
    bool loadPaths = true;
    bool showConfirmFormatButton = false;
    std::array<std::string, 2> cardPaths;

    void parseAndDisplayCard(peripherals::MemoryCard* card);
    void memoryCardWindow(System* sys);
    Region parseRegion(const std::string& filename) const;

   public:
    bool memoryCardWindowOpen = false;
    void displayWindows(System* sys);
};
}  // namespace gui::options