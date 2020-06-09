#include "string.h"
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string_view>
#include <string>

std::vector<std::string_view> split(std::string_view str, std::string_view delim) {
    std::vector<std::string_view> parts;

    std::string_view::size_type pos;
    std::string_view::size_type lastPos = 0;
    std::string_view::size_type length = str.length();

    while (lastPos < length + 1) {
        pos = str.find(delim, lastPos);
        if (pos == std::string_view::npos) {
            pos = length;
        }

        if (pos != lastPos) {
            parts.push_back(str.substr(lastPos, pos - lastPos));
        }

        lastPos = pos + delim.size();
    }
    return parts;
}

std::string_view trim(std::string_view str) {
    const std::string_view pattern = " \f\n\r\t\v";
    std::string_view trimmed;
    if (auto pos = str.find_first_not_of(pattern); pos != std::string_view::npos) {
        trimmed = str.substr(pos);
    }
    if (auto pos = str.find_last_not_of(pattern); pos != std::string_view::npos) {
        trimmed = str.substr(0, pos + 1);
    }

    return trimmed;
}

bool endsWith(const std::string& a, const std::string& b) {
    if (a.length() >= b.length()) {
        return a.compare(a.length() - b.length(), b.length(), b) == 0;
    } else {
        return false;
    }
}

std::string replaceAll(const std::string& str, const std::string& find, const std::string& replace) {
    std::string s = str;
    size_t pos = 0;
    while ((pos = s.find(find, pos)) != std::string::npos) {
        s.replace(pos, find.length(), replace);
        pos += replace.length();
    }
    return s;
}
