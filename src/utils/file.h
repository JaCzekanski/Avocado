#pragma once
#include <vector>
#include <string>

std::string getPath(std::string name);
std::string getFilenameExt(std::string name);
std::string getFilename(std::string name);
std::string getExtension(std::string name);
bool fileExists(std::string name);
std::vector<unsigned char> getFileContents(std::string name);
bool putFileContents(std::string name, std::vector<unsigned char> &contents);
bool putFileContents(std::string name, std::string contents);
std::string getFileContentsAsString(std::string name);
size_t getFileSize(std::string name);
