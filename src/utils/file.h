#pragma once
#include <memory>
#include <string>
#include <vector>

std::string getPath(const std::string& name);
std::string getFilenameExt(const std::string& name);
std::string getFilename(const std::string& name);
std::string getExtension(const std::string& name);
bool fileExists(const std::string& name);
std::vector<unsigned char> getFileContents(const std::string& name);
bool putFileContents(const std::string& name, const std::vector<unsigned char>& contents);
bool putFileContents(const std::string& name, const std::string contents);
std::string getFileContentsAsString(const std::string& name);
size_t getFileSize(const std::string& name);

struct FileDeleter {
    void operator()(FILE* ptr) const {
        if (ptr != nullptr) {
            fclose(ptr);
        }
    }
};

using unique_ptr_file = std::unique_ptr<FILE, FileDeleter>;