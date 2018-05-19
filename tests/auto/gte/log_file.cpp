#include "log_file.h"
#include <sstream>

namespace {
// Fast and unsafe parsing
int getRegNumber(const std::string& line) {
    auto from = line.find('[') + 1;
    auto to = line.find(']');
    std::string sReg = line.substr(from, to - from);
    assert(sReg.size() <= 2);

    return std::stoi(sReg, nullptr, 10);
}

uint32_t getRegValue(const std::string& line) {
    auto from = line.find("0x") + 2;
    std::string sValue = line.substr(from, 8);
    assert(sValue.size() == 8);

    return std::stoul(sValue, nullptr, 16);
}

Entry fastParseRegisterLine(const std::string& line) { return {getRegNumber(line), getRegValue(line)}; }

// Slow regex parsing
Entry parseRegisterLine(const std::string& line) {
    auto registerRegex = std::regex("r\\[(\\d{1,2})\\] = 0x([[:xdigit:]]{8})");

    std::smatch matches;
    std::regex_search(line, matches, registerRegex);

    assert(matches.size() == 3);

    int reg = std::stoi(matches[1], nullptr, 10);
    uint32_t value = std::stoul(matches[2], nullptr, 16);
    return {reg, value};
}

gte::Command parseCommandLine(const std::string& line) {
    auto commandRegex = std::regex(R"(GTE 0x([[:xdigit:]]{2}) .* \(sf=(\d), lm=(\d), tx=(\d), vx=(\d), mx=(\d)\))");

    std::smatch matches;
    std::regex_search(line, matches, commandRegex);

    assert(matches.size() == 7);

    gte::Command cmd(0);

    cmd.cmd = std::stoi(matches[1], nullptr, 16);
    cmd.sf = std::stoi(matches[2], nullptr, 10) == 1;
    cmd.lm = std::stoi(matches[3], nullptr, 10) == 1;
    cmd.mvmvaTranslationVector = std::stoi(matches[4], nullptr, 10);
    cmd.mvmvaMultiplyVector = std::stoi(matches[5], nullptr, 10);
    cmd.mvmvaMultiplyMatrix = std::stoi(matches[6], nullptr, 10);

    return cmd;
}
};  // namespace

std::vector<GteTestCase> parseTestCases(const std::string& file) {
    char lastOperation = '>';
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
            lastOperation = '>';
            testCase.input.push_back(fastParseRegisterLine(line));
        } else if (line[0] == '<') {
            lastOperation = '<';
            testCase.expectedOutput.push_back(fastParseRegisterLine(line));
        } else if (line.substr(0, 3) == "GTE") {
            testCase.cmd = parseCommandLine(line);
            testCase.runCmd = true;
        } else if (line[0] == '=') {
            printf("COMMENT: %s\n", line.c_str());
        }
    }

    if (lastOperation == '<') {
        cases.push_back(testCase);
    }

    return cases;
}