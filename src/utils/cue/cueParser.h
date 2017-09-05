#pragma once
#include "cue.h"
#include "track.h"
#include <regex>
#include <memory>

namespace utils {
class CueParser {
    const std::regex regexFile = std::regex("FILE \"([^\"]+)\" BINARY");
    const std::regex regexTrack = std::regex("TRACK (\\d+) (.*)");
    const std::regex regexIndex = std::regex("INDEX (\\d+) (\\d+):(\\d+):(\\d+)");
    const std::regex regexPregap = std::regex("PREGAP (\\d+):(\\d+):(\\d+)");

    std::string cuePath;
    std::string lastFile;

    Cue cue;
    Track track;

    bool parseFile(std::string& line);
    bool parseTrack(std::string& line);
    bool parseIndex(std::string& line);
    bool parsePregap(std::string& line);
    void addTrackToCue();
    void fixTracksLength();
    Track::Type matchTrackType(const std::string& s) const;

   public:
    std::unique_ptr<Cue> parse(const char* path);
};
}
