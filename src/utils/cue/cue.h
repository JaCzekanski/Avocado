#pragma once
#include "utils/file.h"
#include "utils/string.h"
#include <regex>
#include <memory>
#include <sstream>

namespace utils {
static int SECTOR_SIZE = 2352;

enum class TrackType { DATA, AUDIO };

struct Position {
    int mm = 0;
    int ss = 0;
    int ff = 0;

    static Position fromLba(int lba) {
        int mm = lba / 60 / 75;
        int ss = (lba % (60 * 75)) / 75;
        int ff = lba % 75;
        return {mm, ss, ff};
    }

    std::string toString() const { return string_format("%02d:%02d:%02d", mm, ss, ff); }

    int toLba() const { return (mm * 60 * 75) + (ss * 75) + ff; }

    Position operator+(const Position& p) const { return fromLba(toLba() + p.toLba()); }

    Position operator-(const Position& p) const { return fromLba(toLba() - p.toLba()); }
};

struct Track {
    std::string filename;
    int number = 0;
    TrackType type;

    // Pregap is not included
    // Pause is included in image

    Position pregap;  // size, not position
    Position pause;
    Position start;
    Position end;

    size_t size;
    Position _index0;  // not used

    Position getTrackSize() const { return end - (start - pause - pregap); }
};

struct Cue {
    std::string file;
    std::vector<Track> tracks;

    Position getDiskSize() const {
        Position size = Position::fromLba(2 * 75);
        for (auto t : tracks) {
            size = size + t.getTrackSize();
        }
        return size;
    }

    int getTrackCount() const { return tracks.size(); }

    Position getTrackLength(int track) const { return tracks.at(track).getTrackSize(); }

    void seekTo();
    // read method
};

class CueParser {
    const std::regex regexFile = std::regex("FILE \"([^\"]+)\" BINARY");
    const std::regex regexTrack = std::regex("TRACK (\\d+) (.*)");
    const std::regex regexIndex = std::regex("INDEX (\\d+) (\\d+):(\\d+):(\\d+)");
    const std::regex regexPregap = std::regex("PREGAP (\\d+):(\\d+):(\\d+)");

    std::string cuePath;
    std::string lastFile;

    Cue cue;
    Track track;

    bool parseFile(std::string& line) {
        std::smatch matches;
        regex_search(line, matches, regexFile);
        if (matches.size() != 2) return false;

        lastFile = cuePath + "/" + matches[1].str();
        if (!fileExists(lastFile)) throw std::runtime_error(string_format("File %s does not exist", lastFile.c_str()).c_str());
        return true;
    }

    bool parseTrack(std::string& line) {
        std::smatch matches;
        regex_search(line, matches, regexTrack);
        if (matches.size() != 3) return false;

        track.number = stoi(matches[1].str());
        track.type = matchTrackType(matches[2].str());
        return true;
    }

    bool parseIndex(std::string& line) {
        std::smatch matches;
        regex_search(line, matches, regexIndex);
        if (matches.size() != 5) return false;

        int index = stoi(matches[1].str());
        int mm = stoi(matches[2].str());
        int ss = stoi(matches[3].str());
        int ff = stoi(matches[4].str());
        if (index == 0) {
            track._index0 = {mm, ss, ff};
        } else if (index == 1) {
            track.start = {mm, ss, ff};

            addTrackToCue();
        }
        return true;
    }

    bool parsePregap(std::string& line) {
        std::smatch matches;
        regex_search(line, matches, regexPregap);
        if (matches.size() != 4) return false;

        int mm = stoi(matches[1].str());
        int ss = stoi(matches[2].str());
        int ff = stoi(matches[3].str());
        track.pregap = {mm, ss, ff};
        return true;
    }

    void addTrackToCue() {
        // ignore is track is not completed
        if (track.number == 0) return;

        // Treat index0 as pregap
        // Pregap and index0 are mutually exclusive
        if (track._index0.toLba() != 0) {
            track.pause = track.start - track._index0;
        }
        track.filename = lastFile;
        track.size = getFileSize(lastFile);

        cue.tracks.push_back(track);
        track = Track();
    }

    void fixTracksLength() {
        // Adjust previous track size:
        // - If filename is the same - prev.end = next.start
        // - If filenames are different - prev.end = prev.size / SECTOR_SIZE

        for (size_t i = 0; i < cue.tracks.size(); i++) {
            Track& prev = cue.tracks.at(i);

            if (i == cue.tracks.size() - 1) {
                prev.end = Position::fromLba(prev.size / SECTOR_SIZE);
                return;
            }

            Track& next = cue.tracks.at(i + 1);

            if (prev.filename == next.filename) {
                prev.end = next.start - next.pause;
            } else {
                prev.end = Position::fromLba(prev.size / SECTOR_SIZE);
            }
        }
    }

    TrackType matchTrackType(const std::string& s) const {
        if (s == "MODE2/2352") return TrackType::DATA;
        if (s == "AUDIO") return TrackType::AUDIO;

        throw std::runtime_error(string_format("Unsupported track type %s", s).c_str());
    }

   public:
    std::unique_ptr<Cue> parse(const char* path) {
        cuePath = getPath(path);
        auto contents = getFileContentsAsString(path);
        if (contents.empty()) return nullptr;

        cue.file = path;

        std::stringstream stream;
        stream.str(contents);

        std::string line;
        while (std::getline(stream, line)) {
            if (parseFile(line)) continue;
            if (parseTrack(line)) continue;
            if (parseIndex(line)) continue;
            if (parsePregap(line)) continue;
        }

        addTrackToCue();
        fixTracksLength();

        if (cue.getTrackCount() == 0) return nullptr;
        return std::make_unique<Cue>(cue);
    }
};
}
