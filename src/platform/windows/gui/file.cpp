#include "file.h"
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <algorithm>
#include <cmath>
#include "config.h"
#include "utils/string.h"

#if defined(__cplusplus) && __cplusplus >= 201703L && defined(__has_include) && __has_include(<filesystem>) && !defined(ANDROID)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif

extern bool doHardReset;

namespace gui::file {
bool showHidden = false;
bool openFileWindow = false;
bool readDirectory = false;
std::string currentDirectory;
std::vector<fs::directory_entry> files;

void openFile() {
    if (!openFileWindow) {
        currentDirectory = "";
        readDirectory = true;
    }

    if (readDirectory) {
        readDirectory = false;
        files.clear();

        if (currentDirectory == "") {
            currentDirectory = fs::current_path();
        }

        if (fs::exists(currentDirectory)) {
            currentDirectory = fs::canonical(currentDirectory);
            auto path = fs::path(currentDirectory);

            auto it = fs::directory_iterator(path, fs::directory_options::skip_permission_denied);

            files.push_back(fs::directory_entry(path / ".."));
            for (auto& f : it) {
                if (!showHidden && f.path().filename().string()[0] == '.') continue;
                files.push_back(f);
            }
            std::sort(files.begin(), files.end(), [](const fs::directory_entry& lhs, const fs::directory_entry& rhs) -> bool {
                // true if lhs is less than rhs
                if (lhs.path().filename() == "..") return true;
                if (rhs.path().filename() == "..") return false;

                if (lhs.is_directory() != rhs.is_directory()) return lhs.is_directory() > rhs.is_directory();

                return lhs < rhs;
                // return false;
            });
        }
    }

    openFileWindow = true;
    ImGui::Begin("Open file", &openFileWindow, ImVec2(400.f, 300.f), ImGuiWindowFlags_NoCollapse);

    if (ImGui::InputText("Directory", &currentDirectory, ImGuiInputTextFlags_EnterReturnsTrue)) {
        readDirectory = true;  // Load new directory
    }
    if (ImGui::Checkbox("Show hidden files", &showHidden)) {
        readDirectory = true;
    }

    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::BeginChild("##files", ImVec2(0, 0));
    ImGui::Columns(2, nullptr, false);

    auto formatFileSize = [](long bytes) {
        std::string units = "kMGTPE";

        int unit = 1024;
        if (bytes < unit) return string_format("%4d B", bytes);
        int exp = (int)(std::log(bytes) / std::log(unit));

        char pre = units.at(exp - 1);

        return string_format("%4.f %cB", (float)bytes / std::pow(unit, exp), pre);
    };

    float maxColumnWidth = 0.f;
    for (auto& f : files) {
        auto filename = f.path().filename().string();
        ImVec4 color = ImVec4(1.0, 1.0, 1.0, 1.0);

        if (f.is_directory()) {
            color = ImVec4(0.34, 0.54, 0.56, 1.0);
        }
        if (filename[0] == '.' && filename != "..") {
            color = ImVec4(color.x - 0.3, color.y - 0.3, color.z - 0.3, 1.0);
        }

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        if (ImGui::Selectable(filename.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
            if (f.is_directory()) {
                currentDirectory = f.path();
                readDirectory = true;
            } else if (f.exists()) {
                std::string ext = f.path().extension();
                std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
                printf("Opening %s\n", f.path().c_str());
                if (ext == ".iso" || ext == ".cue" || ext == ".bin" || ext == ".img" || ext == ".chd") {
                    config["iso"] = f.path().string();
                    openFileWindow = false;
                    doHardReset = true;
                }
            }
        }
        ImGui::PopStyleColor();

        ImGui::NextColumn();

        if (!f.is_directory()) {
            std::string fileSize = formatFileSize(f.file_size());

            ImVec2 size = ImGui::CalcTextSize(fileSize.c_str());
            size.x += 8;
            if (size.x > maxColumnWidth) maxColumnWidth = size.x;

            ImGui::Text("%s", fileSize.c_str());
        }

        ImGui::NextColumn();
    }

    ImGui::SetColumnWidth(0, ImGui::GetWindowContentRegionMax().x - maxColumnWidth);
    ImGui::Columns(1);

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    ImGui::End();
}

void close() {}

void displayWindows() {
    if (openFileWindow) openFile();
}
};  // namespace gui::file