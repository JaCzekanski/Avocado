#pragma once
#include <array>
#include <string>
#include <map>
#include <vector>
#include "platform/windows/gui/filesystem.h"

namespace gui::helper {
struct File {
    std::string filename;
    std::string extension;
    fs::directory_entry entry;
    std::string size = "";
    bool isDirectory;
    bool isHidden;
    bool isSupported;

    File(const fs::directory_entry& f, bool isSupported = false);
};

class FileDialog {
    bool autoClose = true;
    bool showHidden = false;
    bool refreshDirectory = true;
    std::string pathInput = "";
    fs::path path;
    std::vector<File> files;

    std::map<std::string, std::string> drivePaths;
    std::vector<const char*> driveNames;
    bool drivesInitialized = false;
    int selectedDrive = 0;

    void getDriveList();
    void readDirectory(const fs::path& _path);

    virtual std::string getDefaultPath();
    virtual bool isFileSupported(const File& f);
    virtual bool onFileSelected(const File& f);

   protected:
    // Config
    bool showOptions = true;
    std::string windowName = "Open file##file_dialog";

   public:
    void display(bool& windowOpen);
};

const std::string formatFileSize(uintmax_t bytes);

void openFileBrowserButton(const std::string& path);
};  // namespace gui::helper