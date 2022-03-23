#include "memory_card.h"
#include <fmt/core.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include "config.h"
#include "../../../../system.h"
#include "utils/string.h"
#include "memory_card/card_formats.h"

namespace gui::options {

char sjisToAscii(uint16_t sjis) {
    uint8_t l = sjis & 0xff;
    uint8_t h = (sjis >> 8) & 0xff;
    if (sjis == 0) return 0;
    if (l == 0x00) {
        if (h == 0x40) return 0;
    }
    if (l == 0x81) {
        // clang-format off
        if (h == 0x40) return ' ';
        else if (h == 0x44) return '.';
        else if (h == 0x46) return ':';
        else if (h == 0x49) return '!';
        else if (h == 0x51) return '_';
        else if (h == 0x5b) return '-';
        else if (h == 0x5e) return '/';
        else if (h == 0x68) return '"';
        else if (h == 0x69) return '(';
        else if (h == 0x6a) return ')';
        else if (h == 0x6d) return '[';
        else if (h == 0x6e) return ']';
        else if (h == 0x7b) return '+';
        else if (h == 0x7c) return ',';
        else if (h == 0x93) return '%';
        // clang-format on
    }
    if (l == 0x82) {
        if (h >= 0x4f && h <= 0x58) return '0' + (h - 0x4f);
        if (h >= 0x60 && h <= 0x79) return 'A' + (h - 0x60);
        if (h >= 0x81 && h <= 0x9a) return 'a' + (h - 0x81);
    }
    //    fmt::print("Unknown S-JIS: 0x{:02x} 0x{:02x}\n", l, h);
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
                char c = data[offset + 0x0a + i];
                if (c == 0) break;
                filename += c;
            }
            Region region = parseRegion(filename);
            std::string gameId;
            if (filename.length() >= 12) {
                gameId = filename.substr(2, 10);  // Strip region
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
            title = trim(title);

            ImGui::Selectable(fmt::format("{:2d}. {} ({})", i, title, gameId).c_str());
            continue;
        }

        ImGui::Selectable(fmt::format("{:2d}. USED", i).c_str());
    }
}

Region MemoryCard::parseRegion(const std::string& filename) const {
    if (filename.length() < 2) return Region::Unknown;

    auto region = filename.substr(0, 2);
    if (region == "BI") {
        return Region::Japan;
    } else if (region == "BE") {
        return Region::Europe;
    } else if (region == "BA") {
        return Region::America;
    } else {
        return Region::Unknown;
    }
}

void MemoryCard::memoryCardWindow(System* sys) {
    if (loadPaths) {
        for (size_t i = 0; i < cardPaths.size(); i++) {
            cardPaths[i] = config.memoryCard[i].path;
        }
        loadPaths = false;
    }
    ImGui::Begin("MemoryCard", &memoryCardWindowOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
    ImGui::BeginTabBar("##memory_select");

    for (size_t i = 0; i < sys->controller->card.size(); i++) {
        if (ImGui::BeginTabItem(fmt::format("Slot {}", i + 1).c_str())) {
            if (ImGui::InputText("Path (enter to load)", &cardPaths[i], ImGuiInputTextFlags_EnterReturnsTrue)) {
                config.memoryCard[i].path = cardPaths[i];
                bus.notify(Event::Controller::MemoryCardSwapped{(int)i});
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

            if (showConfirmFormatButton) {
                if (ImGui::Button("Cancel")) {
                    showConfirmFormatButton = false;
                }
                ImGui::SameLine();
            }

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.8, 0., 0., 1.));
            if (ImGui::Button(showConfirmFormatButton ? "Confirm" : "Format")) {
                if (!showConfirmFormatButton) {
                    showConfirmFormatButton = true;
                } else {
                    memory_card::format(sys->controller->card[i]->data);
                    bus.notify(Event::Controller::MemoryCardContentsChanged{(int)i});
                    showConfirmFormatButton = false;
                }
            }
            ImGui::PopStyleColor();

            ImGui::EndTabItem();
        }
    }

    ImGui::EndTabBar();
    ImGui::End();

    if (!memoryCardWindowOpen) {
        showConfirmFormatButton = false;
    }
}

void MemoryCard::displayWindows(System* sys) {
    if (memoryCardWindowOpen) memoryCardWindow(sys);
}
}  // namespace gui::options