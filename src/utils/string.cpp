#include "string.h"
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <memory>

// http://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
std::string string_format(const std::string fmt_str, ...) {
    int final_n, n = ((int)fmt_str.size()) * 2; /* Reserve two times as much as the length of the fmt_str */
    std::string str;
    std::unique_ptr<char[]> formatted;
    va_list ap;
    while (true) {
        formatted.reset(new char[n]); /* Wrap the plain char array into the unique_ptr */
        strcpy(&formatted[0], fmt_str.c_str());
        va_start(ap, fmt_str);
        final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n)
            n += abs(final_n - n + 1);
        else
            break;
    }
    return std::string(formatted.get());
}

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