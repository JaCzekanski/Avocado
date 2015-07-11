#pragma once
#include <string>
#include <vector>

std::string getPath(std::string name);
std::string getFilename(std::string name);
std::string getExtension(std::string name);
bool fileExists(std::string name);
std::vector<unsigned char> getFileContents(std::string name);
std::string getFileContentsAsString(std::string name);