#include "utils/file.h"
#include <cstdio>

bool fileExists(const std::string &name) {
    FILE *f = fopen(name.c_str(), "r");
    bool exists = false;
    if (f) {
        exists = true;
        fclose(f);
    }
    return exists;
}

std::vector<uint8_t> getFileContents(const std::string &name) {
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

bool putFileContents(const std::string &name, const std::vector<unsigned char> &contents) {
    FILE *f = fopen(name.c_str(), "wb");
    if (!f) return false;

    fwrite(&contents[0], 1, contents.size(), f);

    fclose(f);

    return true;
}

bool putFileContents(const std::string &name, const std::string contents) {
    FILE *f = fopen(name.c_str(), "wb");
    if (!f) return false;

    fwrite(&contents[0], 1, contents.size(), f);

    fclose(f);

    return true;
}

std::string getFileContentsAsString(const std::string &name) {
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

size_t getFileSize(const std::string &name) {
    FILE *f = fopen(name.c_str(), "rb");
    if (!f) return 0;

    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fclose(f);

    return size;
}
