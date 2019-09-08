#include "file.h"
#include <fmt/core.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <algorithm>
#include <cmath>
#include "config.h"
#include "filesystem.h"

namespace gui::file {
bool autoClose = true;
bool showHidden = false;
bool openFileWindow = false;
bool readDirectory = false;
std::string pathInput = "";
fs::path path;
std::vector<fs::directory_entry> files;

const std::array<const char*, 9> supportedFiles = {
    ".iso",      //
    ".cue",      //
    ".bin",      //
    ".img",      //
    ".chd",      //
    ".exe",      //
    ".psexe",    //
    ".psf",      //
    ".minipsf",  //
};

void openFile() {
    if (!openFileWindow) {
        path = fs::path(config["gui"]["lastPath"].get<std::string>());
        readDirectory = true;
    }

    if (readDirectory) {
        readDirectory = false;
        files.clear();

        if (!fs::exists(path)) {
            path = fs::current_path();
        }

        path = fs::canonical(path);

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

            if (fs::is_directory(lhs) != fs::is_directory(rhs)) return fs::is_directory(lhs) > fs::is_directory(rhs);

            return lhs < rhs;
            // return false;
        });

        pathInput = path.string();
    }

    openFileWindow = true;
    ImGui::Begin("Open file", &openFileWindow, ImVec2(400.f, 300.f), -1.f, ImGuiWindowFlags_NoCollapse);

    if (ImGui::Button("Up")) {
        readDirectory = true;
        path /= "..";
    }
    ImGui::SameLine();
    if (ImGui::Button("Home")) {
        readDirectory = true;
        path = fs::current_path();
    }
    ImGui::SameLine();
    if (ImGui::InputText("##Directory", &pathInput, ImGuiInputTextFlags_EnterReturnsTrue)) {
        readDirectory = true;  // Load new directory
        path = fs::path(pathInput);
    }
    if (ImGui::Checkbox("Show hidden files", &showHidden)) {
        readDirectory = true;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Auto close", &autoClose);

    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::BeginChild("##files", ImVec2(0, 0));
    ImGui::Columns(2, nullptr, false);

    auto formatFileSize = [](long bytes) {
        std::string units = "kMGTPE";

        int unit = 1024;
        if (bytes < unit) return fmt::format("{:4d} B", bytes);
        int exp = (int)(std::log(bytes) / std::log(unit));

        char pre = units.at(exp - 1);

        return fmt::format("{:4.0f} {}B", (float)bytes / std::pow(unit, exp), pre);
    };

    float maxColumnWidth = 0.f;
    for (auto& f : files) {
        auto filename = f.path().filename().string();
        ImVec4 color = ImVec4(1.0, 1.0, 1.0, 1.0);
        bool isSupported = false;

        if (fs::is_directory(f)) {
            color = ImVec4(0.34, 0.54, 0.56, 1.0);
        } else if (filename[0] == '.' && filename != "..") {
            color = ImVec4(color.x - 0.3, color.y - 0.3, color.z - 0.3, 1.0);
        } else {
            std::string ext = f.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
            isSupported = std::find(supportedFiles.begin(), supportedFiles.end(), ext) != supportedFiles.end();

            if (!isSupported) {
                color = ImVec4(0.3, 0.3, 0.3, 1.0);
            }
        }

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        if (ImGui::Selectable(filename.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
            if (fs::is_directory(f)) {
                path = f.path();
                readDirectory = true;
            } else if (fs::exists(f) && isSupported) {
                bus.notify(Event::File::Load{f.path().string(), true});
                if (autoClose) openFileWindow = false;
            }
        }
        ImGui::PopStyleColor();

        ImGui::NextColumn();

        if (!fs::is_directory(f)) {
            std::string fileSize = formatFileSize(fs::file_size(f));

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

    // Save last path on window close
    if (!openFileWindow) {
        config["gui"]["lastPath"] = path.string();
    }
}

void close() {}

void displayWindows() {
    if (openFileWindow) openFile();
}
};  // namespace gui::file