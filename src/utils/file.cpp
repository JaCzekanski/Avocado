#include "file.h"

std::string getPath(std::string name) {
    size_t begin = 0, end = name.length();

    size_t slash = name.find_last_of("/\\");
    if (slash != std::string::npos) end = slash + 1;

    return name.substr(begin, end - begin);
}

std::string getFilenameExt(std::string name) {
    size_t begin = 0, end = name.length();

    size_t slash = name.find_last_of("/\\");
    if (slash != std::string::npos) begin = slash + 1;

    return name.substr(begin, end);
}

std::string getFilename(std::string name) {
    size_t begin = 0, end = name.length();

    size_t slash = name.find_last_of("/\\");
    if (slash != std::string::npos) begin = slash + 1;

    size_t dot = name.find_last_of('.');
    if (dot != std::string::npos) end = dot;

    return name.substr(begin, end - begin);
}

std::string getExtension(std::string name) {
    size_t found = name.find_last_of('.');
    if (found == std::string::npos) return "";
    return name.substr(found + 1);
}
