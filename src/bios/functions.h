#pragma once
#include <cstdint>
#include <functional>
#include <string_view>
#include <unordered_map>
#include <vector>

struct System;

namespace bios {

enum class Type {
    INT,     // 32bit HEX - int
    CHAR,    // 8bit char - char
    STRING,  // 32bit pointer to string - char*
    POINTER
};

struct Arg {
    Type type;
    std::string_view name;
};

struct Function {
    std::string_view name;
    std::vector<Arg> args;
    std::function<bool(System* sys)> callback;

    Function(std::string_view prototype, std::function<bool(System* sys)> callback = nullptr);
};

extern const std::unordered_map<uint8_t, Function> A0;
extern const std::unordered_map<uint8_t, Function> B0;
extern const std::unordered_map<uint8_t, Function> C0;
extern const std::array<std::unordered_map<uint8_t, Function>, 3> tables;
extern const std::unordered_map<uint8_t, Function> SYSCALL;
};  // namespace bios
