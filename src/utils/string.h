#pragma once
#include <string>
#include <vector>

std::string string_format(const std::string fmt_str, ...);
std::vector<std::string> split(const std::string& str, const std::string& delim);
std::string trim(const std::string& str);