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

bool fileExists(std::string name) {
    FILE *f = fopen(name.c_str(), "r");
    bool exists = false;
    if (f) {
        exists = true;
        fclose(f);
    }
    return exists;
}

std::vector<uint8_t> getFileContents(std::string name) {
    std::vector<uint8_t> contents;

    FILE *f = fopen(name.c_str(), "rb");
    if (!f) return contents;

    fseek(f, 0, SEEK_END);
    int filesize = ftell(f);
    fseek(f, 0, SEEK_SET);

    contents.resize(filesize);
    fread(&contents[0], 1, filesize, f);

    fclose(f);
    return contents;
}

bool putFileContents(std::string name, std::vector<unsigned char> &contents) {
    FILE *f = fopen(name.c_str(), "wb");
    if (!f) return false;

    fwrite(&contents[0], 1, contents.size(), f);

    fclose(f);

    return true;
}

bool putFileContents(std::string name, std::string contents) {
    FILE *f = fopen(name.c_str(), "wb");
    if (!f) return false;

    fwrite(&contents[0], 1, contents.size(), f);

    fclose(f);

    return true;
}

std::string getFileContentsAsString(std::string name) {
    std::string contents;

    FILE *f = fopen(name.c_str(), "rb");
    if (!f) return contents;

    fseek(f, 0, SEEK_END);
    int filesize = ftell(f);
    fseek(f, 0, SEEK_SET);

    contents.resize(filesize);
    fread(&contents[0], 1, filesize, f);

    fclose(f);
    return contents;
}

size_t getFileSize(std::string name) {
    FILE *f = fopen(name.c_str(), "rb");
    if (!f) return 0;

    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fclose(f);

    return size;
}
