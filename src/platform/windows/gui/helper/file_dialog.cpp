#include "file_dialog.h"
#include <fmt/core.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <platform/windows/utils/platform_tools.h>
#include <imgui_internal.h>
#include "config.h"
#include "platform/windows/gui/filesystem.h"
#include "utils/file.h"
#include "utils/string.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace gui::helper {

bool isHidden(const fs::directory_entry& f) {
    auto filename = f.path().filename().string();
    return filename[0] == '.' && filename != "..";
}

File::File(const fs::directory_entry& f, bool isSupported) : entry(f), isSupported(isSupported) {
    filename = f.path().filename().string();
    extension = f.path().extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), tolower);
    isDirectory = fs::is_directory(f);

    if (!isDirectory) {
        size = formatFileSize(fs::file_size(f));
    }

    isHidden = gui::helper::isHidden(f);
}

void FileDialog::getDriveList() {
    drivesInitialized = true;

    drivePaths["Avocado"] = avocado::PATH_USER;

#ifdef _WIN32
    DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 32; i++) {
        if (drives & (1 << i)) {
            char letter = 'A' + i;
            auto name = fmt::format("{}:", letter);
            auto path = fmt::format("{}:\\", letter);
            drivePaths[name] = path;
        }
    }
#endif

#ifdef __APPLE__
    // Get list of drives by iterating /Volumes
    auto volumes = fs::directory_iterator("/Volumes", fs::directory_options::skip_permission_denied);
    for (auto& volume : volumes) {
        try {
            if (!fs::exists(volume) || !fs::is_directory(volume) || isHidden(volume)) {
                continue;
            }
            drivePaths[volume.path().filename().string()] = volume.path().string();
        } catch (fs::filesystem_error& err) {
            fmt::print("{}\n", err.what());
        }
    }
#endif

#ifdef __linux__
    auto mounts = getFileContentsAsString("/proc/mounts");
    if (!mounts.empty()) {
        std::string line;
        std::istringstream is(mounts);
        while (getline(is, line)) {
            auto components = split(line, " ");
            // Get only mountpoints with real devices
            if (components.size() < 2 || components[0].rfind("/dev", 0) != 0) {
                continue;
            }

            auto volume = fs::path(std::string(components[1]));
            auto name = volume.filename().string();
            if (name.empty()) {
                name = "/";
            }
            drivePaths[name] = volume.string();
        }
    }
#endif

#ifdef __ANDROID__
    if (hasExternalStoragePermission()) {
        drivePaths["SD card"] = "/sdcard";
    }
#endif

    for (auto const& [name, path] : drivePaths) {
        driveNames.push_back(name.c_str());
    }
}

void FileDialog::readDirectory(const fs::path& _path) {
    refreshDirectory = false;
    files.clear();

    if (!fs::exists(_path)) {
        path = fs::current_path();
    }

    path = fs::canonical(_path);

    auto it = fs::directory_iterator(path, fs::directory_options::skip_permission_denied);

    files.push_back(fs::directory_entry(path / ".."));
    for (auto& f : it) {
        try {
            files.push_back(File(f, isFileSupported(f)));
        } catch (fs::filesystem_error& err) {
            fmt::print("{}\n", err.what());
        }
    }

    std::sort(files.begin(), files.end(), [](const File& lhs, const File& rhs) -> bool {
        // true if lhs is less than rhs
        if (lhs.filename == "..") return true;
        if (rhs.filename == "..") return false;

        if (lhs.isDirectory != rhs.isDirectory) return lhs.isDirectory > rhs.isDirectory;

        return lhs.filename < rhs.filename;
        // return false;
    });

    pathInput = path.string();
}

std::string FileDialog::getDefaultPath() { return config.gui.lastPath; }

bool FileDialog::isFileSupported(const File& f) {
    if (mode == Mode::SelectDirectory) {
        return false;
    } else {
        return true;
    }
}

bool FileDialog::onFileSelected(const File& f) { return false; }

// Close on false
void FileDialog::display(bool& windowOpen) {
    if (!drivesInitialized) {
        getDriveList();
    }
    if (refreshDirectory) {
        if (path.empty()) {
            path = fs::path(getDefaultPath());
        }
        readDirectory(path);
    }

    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin(windowName.c_str(), &windowOpen, ImGuiWindowFlags_NoCollapse);

    if (ImGui::Button("Up")) {
        readDirectory(path / "..");
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    if (ImGui::Combo("##drive", &selectedDrive, driveNames.data(), driveNames.size())) {
        readDirectory(drivePaths[driveNames[selectedDrive]]);
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##Directory", &pathInput, ImGuiInputTextFlags_EnterReturnsTrue)) {
        readDirectory(fs::path(pathInput));
    }

    openFileBrowserButton(path.string());
    if (showOptions) {
        ImGui::SameLine();
        ImGui::Checkbox("Show hidden files", &showHidden);
        ImGui::SameLine();
        ImGui::Checkbox("Auto close", &autoClose);
    }

    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImGui::BeginChild("##files", ImVec2(0, (mode == Mode::OpenFile) ? 0 : -ImGui::GetFrameHeightWithSpacing()));
    ImGui::Columns(2, nullptr, false);

    float maxColumnWidth = 0.f;
    for (auto& f : files) {
        if (!showHidden && f.isHidden) continue;

        ImVec4 color = ImVec4(1.f, 1.f, 1.f, 1.f);
        if (f.isDirectory) {
            color = ImVec4(0.34f, 0.54f, 0.56f, 1.f);
        } else if (f.isHidden) {
            color = ImVec4(color.x - 0.3f, color.y - 0.3f, color.z - 0.3f, 1.f);
        } else if (!f.isSupported) {
            color = ImVec4(0.3f, 0.3f, 0.3f, 1.f);
        }

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        if (ImGui::Selectable(f.filename.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
            if (f.isDirectory) {
                path = f.entry.path();
                refreshDirectory = true;

                if (mode == Mode::SaveFile) {
                    saveFileName = "";
                }
            } else if (f.isSupported) {
                if (fs::exists(f.entry)) {
                    if (mode == Mode::OpenFile) {
                        bool fileHandled = onFileSelected(f);

                        if (fileHandled) {
                            if (autoClose) windowOpen = false;
                        }
                    } else if (mode == Mode::SaveFile) {
                        saveFileName = f.filename;
                    }
                } else {
                    refreshDirectory = true;
                }
            }
        }
        ImGui::PopStyleColor();
        ImGui::NextColumn();

        ImVec2 size = ImGui::CalcTextSize(f.size.c_str());
        size.x += 8;
        if (size.x > maxColumnWidth) maxColumnWidth = size.x;

        ImGui::Text("%s", f.size.c_str());
        ImGui::NextColumn();
    }

    ImGui::SetColumnWidth(0, ImGui::GetWindowContentRegionMax().x - maxColumnWidth);
    ImGui::Columns(1);
    ImGui::EndChild();

    if (mode == Mode::SelectDirectory) {
        if (ImGui::Button("Select current directory")) {
            bool fileHandled = onFileSelected(File(fs::directory_entry(path)));
            if (fileHandled) {
                windowOpen = false;
            }
        }
    } else if (mode == Mode::SaveFile) {
        ImGui::InputText("##saveFileName", &saveFileName);
        ImGui::SameLine();

        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, saveFileName.empty());
        if (ImGui::Button("Save")) {
            // Hack
            auto f = File(fs::directory_entry(path));
            f.filename = getFilename(saveFileName);
            f.extension = getExtension(saveFileName);
            if (f.extension != "") {
                f.extension = std::string(".") + f.extension;
            }
            bool fileHandled = onFileSelected(f);
            if (fileHandled) {
                windowOpen = false;
            }
        }
        ImGui::PopItemFlag();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    ImGui::End();

    if (!windowOpen) {
        refreshDirectory = true;
    }
}

const std::string formatFileSize(uintmax_t bytes) {
    const std::string units = "kMGTPE";

    size_t unit = 1024;
    if (bytes < unit) return fmt::format("{:4d} B", bytes);
    int exp = (int)(std::log(bytes) / std::log(unit));

    char pre = units.at(exp - 1);

    return fmt::format("{:4.0f} {}B", (float)bytes / std::pow(unit, exp), pre);
}

void openFileBrowserButton(const std::string& path) {
#if defined(__APPLE__)
    if (ImGui::Button("Reveal in Finder")) {
        openFileBrowser(path);
    }
#elif defined(_WIN32)
    if (ImGui::Button("Open in Explorer")) {
        openFileBrowser(path);
    }
#elif defined(__linux__)
    if (ImGui::Button("Open in file explorer")) {
        openFileBrowser(path);
    }
#endif
};
};  // namespace gui::helper
