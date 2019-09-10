#include "utils/file.h"
#include <SDL.h>

bool fileExists(const std::string &name) {
    SDL_RWops *f = SDL_RWFromFile(name.c_str(), "r");
    bool exists = false;
    if (f) {
        exists = true;
        SDL_RWclose(f);
    }
    return exists;
}

std::vector<uint8_t> getFileContents(const std::string &name) {
    std::vector<uint8_t> contents;

    SDL_RWops *f = SDL_RWFromFile(name.c_str(), "rb");
    if (!f) return contents;

    SDL_RWseek(f, 0, SEEK_END);
    int filesize = SDL_RWtell(f);
    SDL_RWseek(f, 0, SEEK_SET);

    contents.resize(filesize);
    SDL_RWread(f, &contents[0], 1, filesize);

    SDL_RWclose(f);
    return contents;
}

bool putFileContents(const std::string &name, const std::vector<unsigned char> &contents) {
    SDL_RWops *f = SDL_RWFromFile(name.c_str(), "wb");
    if (!f) return false;

    SDL_RWwrite(f, &contents[0], 1, contents.size());

    SDL_RWclose(f);

    return true;
}

bool putFileContents(const std::string &name, const std::string contents) {
    SDL_RWops *f = SDL_RWFromFile(name.c_str(), "wb");
    if (!f) return false;

    SDL_RWwrite(f, &contents[0], 1, contents.size());

    SDL_RWclose(f);

    return true;
}

std::string getFileContentsAsString(const std::string &name) {
    std::string contents;

    SDL_RWops *f = SDL_RWFromFile(name.c_str(), "rb");
    if (!f) return contents;

    SDL_RWseek(f, 0, SEEK_END);
    int filesize = SDL_RWtell(f);
    SDL_RWseek(f, 0, SEEK_SET);

    contents.resize(filesize);
    SDL_RWread(f, &contents[0], 1, filesize);

    SDL_RWclose(f);
    return contents;
}

size_t getFileSize(const std::string &name) {
    SDL_RWops *f = SDL_RWFromFile(name.c_str(), "rb");
    if (!f) return 0;

    SDL_RWseek(f, 0, SEEK_END);
    int size = SDL_RWtell(f);
    SDL_RWclose(f);

    return size;
}
