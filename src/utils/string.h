#pragma once
#include <string_view>
#include <vector>

std::vector<std::string_view> split(std::string_view str, std::string_view delim);
std::string_view trim(std::string_view str);