#include <climits>
#include <string>
#include <fstream>
#include <vector>
#include "utils/string.h"
#define MAXCHARS 2048

struct GamesharkCode {
    uint32_t opcode;
    uint32_t address;
    uint32_t value;
};

struct GamesharkCheat {
    std::string name = "Unnamed";
    std::vector<GamesharkCode> codes;
    bool enabled = false;
};

struct Gameshark {
   public:
    static Gameshark *getInstance() {
        static Gameshark instance;
        return &instance;
    }

    std::vector<GamesharkCheat> gamesharkCheats;

    void readFile(std::string filename) {
        gamesharkCheats.clear();
        std::ifstream ifstream(filename);
        GamesharkCheat gamesharkCheat;
        bool cheatValid = false;
        while (!ifstream.eof()) {
            std::string line;
            std::getline(ifstream, line);
            line = trim(line);
            if (line[0] == '#') {
                if (cheatValid) {
                    gamesharkCheats.push_back(gamesharkCheat);
                    gamesharkCheat = GamesharkCheat();
                    cheatValid = false;
                }
                gamesharkCheat.name = line.substr(1);
                cheatValid = true;
            } else if (line == "") {
                if (cheatValid) {
                    gamesharkCheats.push_back(gamesharkCheat);
                    gamesharkCheat = GamesharkCheat();
                    cheatValid = false;
                }
            } else {
                char *end;
                GamesharkCode code;
                code.opcode = std::strtoul(line.substr(0, 2).c_str(), &end, 16);
                if (code.opcode == UINT_MAX) {
                    continue;
                }
                code.address = std::strtoul(line.substr(2, 6).c_str(), &end, 16);
                if (code.address == UINT_MAX) {
                    continue;
                }
                std::string space = line.substr(8, 1);
                if (space != " ") {
                    continue;
                }
                code.value = std::strtoul(line.substr(9, 4).c_str(), &end, 16);
                if (code.value == UINT_MAX) {
                    continue;
                }
                gamesharkCheat.codes.push_back(code);
                cheatValid = true;
            }
        }
        if (cheatValid) {
            gamesharkCheats.push_back(gamesharkCheat);
        }
        ifstream.close();
    }
};