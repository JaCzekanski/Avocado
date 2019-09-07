#include "string.h"
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <memory>

std::vector<std::string> split(const std::string& str, const std::string& delim) {
    std::vector<std::string> parts;

    std::string::size_type pos;
    std::string::size_type lastPos = 0;
    std::string::size_type length = str.length();

    while (lastPos < length + 1) {
        pos = str.find(delim, lastPos);
        if (pos == std::string::npos) {
            pos = length;
        }

        if (pos != lastPos) {
            parts.push_back(std::string(str.data() + lastPos, pos - lastPos));
        }

        lastPos = pos + delim.size();
    }
    return parts;
}

std::string trim(const std::string& str) {
    const std::string pattern = " \f\n\r\t\v";
    std::string trimmed;
    trimmed = str.substr(str.find_first_not_of(pattern));
    trimmed = str.substr(0, str.find_last_not_of(pattern) + 1);

    return trimmed;
}