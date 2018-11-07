#pragma once
#include <memory>
#include <optional>
#include <regex>
#include "cue.h"
#include "track.h"

namespace utils {
class ParsingException : public std::runtime_error {
   public:
    ParsingException(const std::string& s) : std::runtime_error(s) {}
};
class CueParser {
    const std::regex regexFile = std::regex(R"(FILE \"([^\"]+)\" (.*))");
    const std::regex regexTrack = std::regex(R"(TRACK (\d+) (.*))");
    const std::regex regexIndex = std::regex(R"(INDEX (\d+) (\d+):(\d+):(\d+))");
    const std::regex regexPregap = std::regex(R"(PREGAP (\d+):(\d+):(\d+))");
    const std::regex regexPostgap = std::regex(R"(POSTGAP (\d+):(\d+):(\d+))");

    std::string cuePath;
    std::string lastFile;
    std::string lastFileType;

    Cue cue;
    Track track;

    bool parseFile(std::string& line);
    bool parseTrack(std::string& line);
    bool parseIndex(std::string& line);
    bool parsePregap(std::string& line);
    bool parsePostgap(std::string& line);
    void addTrackToCue();
    void fixTracksLength();
    Track::Type matchTrackType(const std::string& s) const;

   public:
    std::optional<Cue> parse(const char* path);
};
}  // namespace utils
