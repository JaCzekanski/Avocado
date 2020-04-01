#include "toasts.h"
#include <fmt/core.h>
#include <imgui.h>
#include "config.h"
#include "utils/string.h"

using namespace std::chrono_literals;

namespace gui {
Toasts::Toasts() {
    busToken = bus.listen<Event::Gui::Toast>([&](auto e) {
        fmt::print("{}\n", e.message);
        toasts.push_back({e.message, std::chrono::steady_clock::now() + 2s});
    });
}

Toasts::~Toasts() { bus.unlistenAll(busToken); }

std::string Toasts::getToasts() {
    std::string msg = "";
    if (!toasts.empty()) {
        auto now = std::chrono::steady_clock::now();

        for (auto it = toasts.begin(); it != toasts.end();) {
            auto toast = *it;
            bool isLast = it == toasts.end();

            if (now > toast.expire) {
                it = toasts.erase(it);
            } else {
                msg += toast.msg;
                if (!isLast) {
                    msg += "\n";
                }
                ++it;
            }
        }
    }
    return msg;
}

void Toasts::display() {
    if (auto toasts = getToasts(); !toasts.empty()) {
        const float margin = 16;
        auto displaySize = ImGui::GetIO().DisplaySize;

        auto pos = ImVec2(displaySize.x - margin, displaySize.y - margin);
        ImGui::SetNextWindowPos(pos, 0, ImVec2(1.f, 1.f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::Begin("##toast", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextUnformatted(toasts.c_str());
        ImGui::End();
        ImGui::PopStyleVar();
    }
}
}  // namespace gui