#include <catch.hpp>
#include <regex>
#include <sstream>
#include "cpu/gte/gte.h"
#include "utils/file.h"
#include "utils/string.h"

struct Entry {
    int reg;
    uint32_t data;
};

struct GteTestCase {
    std::vector<Entry> input;
    std::vector<Entry> expectedOutput;
    gte::Command cmd;

    GteTestCase() : cmd(0) { cmd.cmd = 0x40; }
};

Entry parseRegisterLine(const std::string& line) {
    auto registerRegex = std::regex("r\\[(\\d{1,2})\\] = 0x([[:xdigit:]]{8})");

    std::smatch matches;
    std::regex_search(line, matches, registerRegex);

    REQUIRE(matches.size() == 3);

    int reg = std::stoi(matches[1], nullptr, 10);
    uint32_t value = std::stoul(matches[2], nullptr, 16);
    return {reg, value};
}

gte::Command parseCommandLine(const std::string& line) {
    auto commandRegex = std::regex("GTE 0x([[:xdigit:]]{2}) \\(sf=(\\d), lm=(\\d), tx=(\\d), vx=(\\d), mx=(\\d)\\)");

    std::smatch matches;
    std::regex_search(line, matches, commandRegex);

    REQUIRE(matches.size() == 7);

    gte::Command cmd(0);

    cmd.cmd = std::stoi(matches[1], nullptr, 16);
    cmd.sf = std::stoi(matches[2], nullptr, 10) == 1;
    cmd.lm = std::stoi(matches[3], nullptr, 10) == 1;
    cmd.mvmvaMultiplyVector = std::stoi(matches[4], nullptr, 10);
    cmd.mvmvaTranslationVector = std::stoi(matches[5], nullptr, 10);
    cmd.mvmvaMultiplyMatrix = std::stoi(matches[6], nullptr, 10);

    return cmd;
}

std::vector<GteTestCase> parseTestCases(const std::string& file) {
    char lastOperation = ' ';
    std::vector<GteTestCase> cases;
    GteTestCase testCase;
    auto contents = getFileContentsAsString(file);

    std::stringstream stream;
    std::string line;
    stream.str(contents);
    while (std::getline(stream, line)) {
        // -------------- GTE 0x00 separator
        // > input
        // < expected output
        // GTE 0x00 (sf=0, lm=0, tx=0, vx=3, mx=1)

        if (line.empty()) continue;

        if (line[0] == '>' && lastOperation != '>') {
            cases.push_back(testCase);
            testCase = GteTestCase();
        }

        if (line[0] == '>') {
            testCase.input.push_back(parseRegisterLine(line));
        } else if (line[0] == '<') {
            testCase.expectedOutput.push_back(parseRegisterLine(line));
        } else if (line.substr(0, 3) == "GTE") {
            testCase.cmd = parseCommandLine(line);
        }
    }

    return cases;
}

TEST_CASE("All commands tested on real hardware", "[GTE]") {
    static auto testCases = parseTestCases("tests/gte/all_psx.txt");
    FILE* f = fopen("tests/gte/all_emu.txt", "wb");

    GTE gte;
    for (auto& testCase : testCases) {
        //        SECTION(string_format("GTE cmd 0x%02x", testCase.cmd.cmd)) {
        for (auto& input : testCase.input) {
            gte.write(input.reg, input.data);
            fprintf(f, "> r[%d] = 0x%08x\n", input.reg, input.data);
        }

        if (testCase.cmd.cmd != 0x40) {
            fprintf(f, "GTE 0x%02x (sf=%d, lm=%d, tx=%d, vx=%d, mx=%d)\n", testCase.cmd.cmd, testCase.cmd.sf, testCase.cmd.lm,
                    testCase.cmd.mvmvaTranslationVector, testCase.cmd.mvmvaMultiplyVector, testCase.cmd.mvmvaMultiplyMatrix);
            gte.command(testCase.cmd);
        }

        for (int i = 0; i < 64; i++) {
            uint32_t data = gte.read(i);
            fprintf(f, "< r[%d] = 0x%08x\n", i, data);
        }
        //            for (auto& expected : testCase.expectedOutput) {
        //                CHECK(gte.read(expected.reg) == expected.data);
        //            }
        //        }
    }
    fclose(f);
}