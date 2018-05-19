#include "cpu/gte/gte.h"
#include "log_file.h"
#include "utils/file.h"

void printHelp() {
    printf(R"(
usage: avocado_autotest logfile.log
  --ignore-flag - ignore reg[63] differences
  --show-input  - print input data on failed assertion
  --help        - print help
)");
}

void printInfo(GteTestCase& testCase, int testNumber) {
    printf("\n\n============== ASSERTION FAILED =============\n");
    printf("Test number %d\n", testNumber);
    if (testCase.runCmd) {
        printf("Command: GTE 0x%02x (sf=%d, lm=%d, tx=%d, vx=%d, mx=%d)\n", testCase.cmd.cmd, testCase.cmd.sf, testCase.cmd.lm,
               testCase.cmd.mvmvaTranslationVector, testCase.cmd.mvmvaMultiplyVector, testCase.cmd.mvmvaMultiplyMatrix);
    }

    printf("\nDifferences:\n");
    printf("          received    -  expected\n");
}

void printInput(GteTestCase& testCase) {
    printf("\n\nInput:\n");
    for (auto& input : testCase.input) {
        printf("> r[%d] = 0x%08x\n", input.reg, input.data);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printHelp();
        return 0;
    }

    std::string logfile;
    bool showInput = false;
    bool ignoreFlag = false;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--ignore-flag") == 0) {
            ignoreFlag = true;
            continue;
        }
        if (strcmp(argv[i], "--show-input") == 0) {
            showInput = true;
            continue;
        }
        if (strcmp(argv[i], "--help") == 0) {
            printHelp();
            return 0;
        }

        if (i == argc - 1) {
            logfile = argv[i];
        }
    }

    if (!fileExists(logfile)) {
        printf("File %s does not exist.\n", logfile.c_str());
        return 1;
    }

    printf("Using file %s\n", logfile.c_str());
    auto testCases = parseTestCases(logfile);
    printf("Test cases found: %d\n", testCases.size());

    GTE gte;

    int testsFailed = 0;
    int testsSuccessful = 0;
    int assertionsFailed = 0;
    int assertionsSuccessful = 0;

    int testNumber = 1;

    for (auto& testCase : testCases) {
        bool testFailed = false;

        for (auto& input : testCase.input) {
            gte.write(input.reg, input.data);
        }

        if (testCase.runCmd) {
            gte.command(testCase.cmd);
        }

        for (auto& expected : testCase.expectedOutput) {
            auto data = gte.read(expected.reg);

            if (data == expected.data) {
                assertionsSuccessful++;
                continue;
            }

            assertionsFailed++;
            if (!testFailed) {
                if (expected.reg == 63 && ignoreFlag) continue;

                testFailed = true;
                printInfo(testCase, testNumber);
            }
            printf("  r[%2d]:  0x%08x  -  0x%08x\n", expected.reg, data, expected.data);
        }

        if (testFailed && showInput) {
            printInput(testCase);
        }

        if (testFailed) {
            testsFailed++;
        } else {
            testsSuccessful++;
        }

        testNumber++;
    }
    printf("\n\nStats:\n");
    printf("Tests total: %d\n", testsFailed + testsSuccessful);
    printf("Tests successful: %d\n", testsSuccessful);
    printf("Tests failed: %d\n", testsFailed);
    printf("Assertions total: %d\n", assertionsFailed + assertionsSuccessful);
    printf("Assertions successful: %d\n", assertionsSuccessful);
    printf("Assertions failed: %d\n", assertionsFailed);

    if (testsFailed > 0) return 1;

    return 0;
}