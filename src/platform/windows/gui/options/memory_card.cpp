#include "memory_card.h"
#include <fmt/core.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include "config.h"
#include "system.h"

namespace gui::options {

char sjisToAscii(uint16_t sjis) {
    uint8_t l = sjis & 0xff;
    uint8_t h = (sjis >> 8) & 0xff;
    if (sjis == 0) return 0;
    if (l == 0x81) {
        // if (h == 0x14) return '-';
        if (h == 0x5b) return '-';
        if (h == 0x40) return ' ';
        if (h == 0x46) return ':';
        if (h == 0x49) return '!';
        if (h == 0x5e) return '/';
        if (h == 0x6d) return '[';
        if (h == 0x6e) return ']';
        if (h == 0x69) return '(';
        if (h == 0x6a) return ')';
        if (h == 0x7b) return '+';
        if (h == 0x7c) return ',';
        if (h == 0x93) return '%';
        // if (h >= 0x43 && h <= 0x97) return ' ' + (h-0x43);
    }
    if (l == 0x82) {
        if (h >= 0x4f && h <= 0x58) return '0' + (h - 0x4f);
        if (h >= 0x60 && h <= 0x79) return 'A' + (h - 0x60);
        if (h >= 0x81 && h <= 0x9a) return 'a' + (h - 0x81);
    }
    // fmt::print("Unknown S-JIS: 0x{:02x} 0x{:02x}\n", l, h);
    return '?';
}

void MemoryCard::parseAndDisplayCard(peripherals::MemoryCard* card) {
    auto data = card->data;
    auto read32
        = [data](size_t pos) -> uint32_t { return data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24); };

    const int BLOCKS = 15;
    const auto FRAME_SIZE = 0x80;
    const auto BLOCK_SIZE = FRAME_SIZE * 64;

    ImGui::Text("Contents:");

    enum BlockNum { First = 1, Middle = 2, Last = 3 };

    std::string names[BLOCKS];
    int links[BLOCKS];

    for (size_t i = 1; i < BLOCKS + 1; i++) {
        size_t offset = 0x80 * i;
        uint32_t state = read32(offset + 0);

        bool inUse = false;
        if (state >= 0x51 && state <= 0x53) {
            if (state == 0x51) {
                links[i - 1] = -1;
            }
            inUse = true;
        }

        if (!inUse) {
            ImGui::Selectable(fmt::format("{:2d}. ---", i).c_str());
            continue;
        }

        // state == 0x51 - block in use, first block
        // state == 0x52 - block in use, middle block
        // state == 0x53 - block in use, end block

        if (state == 0x51) {
            uint32_t size = read32(offset + 4) / 1024;
            std::string filename;
            for (int i = 0; i < 20; i++) {
                filename += data[offset + 0x0a + i];
            }

            // Parse block
            std::string title;

            for (int s = 0; s < 32; s++) {
                size_t offset = BLOCK_SIZE * i + 4;
                uint16_t sjis = data[offset + s * 2] | (data[offset + s * 2 + 1] << 8);

                char c = sjisToAscii(sjis);
                if (c == 0) break;
                title += c;
            }

            ImGui::Selectable(fmt::format("{:2d}. {} ({})", i, title, filename).c_str());
            continue;
        }

        ImGui::Selectable(fmt::format("{:2d}. USED", i).c_str());
    }
}
void MemoryCard::memoryCardWindow(System* sys) {
    if (loadPaths) {
        for (size_t i = 0; i < cardPaths.size(); i++) {
            cardPaths[i] = config["memoryCard"][std::to_string(i + 1)];
        }
        loadPaths = false;
    }
    ImGui::Begin("MemoryCard", &memoryCardWindowOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
    ImGui::BeginTabBar("##memory_select");

    for (size_t i = 0; i < sys->controller->card.size(); i++) {
        if (ImGui::BeginTabItem(fmt::format("Slot {}", i + 1).c_str())) {
            if (ImGui::InputText("Path", &cardPaths[i])) {
                config["memoryCard"][std::to_string(i + 1)] = cardPaths[i];
            }
            // TODO: Reload contents on change?
            // TODO: Check if card exists, override or reload?

            bool inserted = sys->controller->card[i]->inserted;
            if (ImGui::Checkbox("Inserted", &inserted)) {
                sys->controller->card[i]->inserted = inserted;
            }

            // ImGui::BeginChild("Contents");
            parseAndDisplayCard(sys->controller->card[i].get());
            // ImGui::EndChild();

            ImGui::EndTabItem();
        }
    }

    ImGui::EndTabBar();
    ImGui::End();
}

void MemoryCard::displayWindows(System* sys) {
    if (memoryCardWindowOpen) memoryCardWindow(sys);
}
}  // namespace gui::options