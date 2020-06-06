#pragma once
#include <string_view>
#include <vector>

std::vector<std::string_view> split(std::string_view str, std::string_view delim);
std::string_view trim(std::string_view str);
bool endsWith(const std::string& a, const std::string& b);