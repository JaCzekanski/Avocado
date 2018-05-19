#pragma once
#include <iostream>
#include <regex>
#include "cpu/gte/command.h"
#include "utils/file.h"

#define assert(condition)                                                                                          \
    do {                                                                                                           \
        if (!(condition)) {                                                                                        \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ << " line " << __LINE__ << std::endl; \
            std::terminate();                                                                                      \
        }                                                                                                          \
    } while (false)

struct Entry {
    int reg;
    uint32_t data;
};

struct GteTestCase {
    std::vector<Entry> input;
    std::vector<Entry> expectedOutput;
    gte::Command cmd;
    bool runCmd;

    GteTestCase() : cmd(0), runCmd(false) {}
};

std::vector<GteTestCase> parseTestCases(const std::string& file);