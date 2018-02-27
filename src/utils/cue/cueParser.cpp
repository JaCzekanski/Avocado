#include "cueParser.h"
#include <sstream>
#include "utils/file.h"
#include "utils/string.h"

namespace utils {

bool CueParser::parseFile(std::string& line) {
    std::smatch matches;
    regex_search(line, matches, regexFile);
    if (matches.size() != 2) return false;

    lastFile = cuePath + "/" + matches[1].str();
    if (!fileExists(lastFile)) throw std::runtime_error(string_format("File %s does not exist", lastFile.c_str()));
    return true;
}

bool CueParser::parseTrack(std::string& line) {
    std::smatch matches;
    regex_search(line, matches, regexTrack);
    if (matches.size() != 3) return false;

    track.number = stoi(matches[1].str());
    track.type = matchTrackType(matches[2].str());
    return true;
}

bool CueParser::parseIndex(std::string& line) {
    std::smatch matches;
    regex_search(line, matches, regexIndex);
    if (matches.size() != 5) return false;

    int index = stoi(matches[1].str());
    int mm = stoi(matches[2].str());
    int ss = stoi(matches[3].str());
    int ff = stoi(matches[4].str());
    if (index == 0) {
        index0 = {mm, ss, ff};
    } else if (index == 1) {
        track.start = {mm, ss, ff};

        if (track.number == 1 && track.type == Track::Type::DATA) {
            // Even though Index1 points to 00:00:00
            // binary file of first track (data) cointains data from 00:02:00, not 00:00:00.
            // Two seconds are missing and this code adapts for that
        }

        addTrackToCue();
    }
    return true;
}

bool CueParser::parsePregap(std::string& line) {
    std::smatch matches;
    regex_search(line, matches, regexPregap);
    if (matches.size() != 4) return false;

    int mm = stoi(matches[1].str());
    int ss = stoi(matches[2].str());
    int ff = stoi(matches[3].str());
    track.pregap = {mm, ss, ff};
    return true;
}

void CueParser::addTrackToCue() {
    // ignore if track is not completed
    if (track.number == 0) return;

    // Treat index0 as pregap
    // Pregap and index0 are mutually exclusive
    if (track.number > 1 && track.pregap.toLba() == 0) {
        track.pause = track.start - index0;
    }
    track.filename = lastFile;
    track.size = getFileSize(lastFile);

    track.start = globalOffset + track.start + track.pregap;

    cue.tracks.push_back(track);
    track = Track();
    index0 = Position(0, 0, 0);
}

void CueParser::fixTracksLength() {
    if (cue.getTrackCount() == 1) {
        Track& track = cue.tracks[0];

        track.offsetInFile = 0;
        track.end = Position::fromLba(track.size / Track::SECTOR_SIZE);
        return;
    }

    Position lastEnd = globalOffset;

    for (int i = 0; i < cue.getTrackCount(); i++) {
        Track& t = cue.tracks[i];
        // last track
        // fix length for last track
        if (i == cue.getTrackCount() - 1) {
            t.end = lastEnd + Position::fromLba(t.size / Track::SECTOR_SIZE);

            break;
        }

        Track& n = cue.tracks[i + 1];

        if (t.filename == n.filename) {
            t.end = n.start - (n.pregap + n.pause) + t.pregap;
            n.offsetInFile = t.end.toLba() * Track::SECTOR_SIZE;
        } else {
            t.offsetInFile = 0;
            t.end = Position::fromLba(t.size / Track::SECTOR_SIZE) + lastEnd;
            n.start = n.start + t.end;

            lastEnd = t.end;
        }

        //        Track& prev = cue.tracks[i - 1];
        //        Track& next = cue.tracks[i];
        //
        //        // Separate bin files
        //        if (next.filename != prev.filename) {
        //            prev.end = prev.start + Position::fromLba(prev.size / Track::SECTOR_SIZE);
        //            next.start = next.start + prev.end;
        //            next.offsetInFile = 0;
        //        } else {
        //            // Single bin
        //            prev.end = next.start - (next.pregap + next.pause);
        //            next.offsetInFile = prev.end.toLba() * Track::SECTOR_SIZE;
        //        }
    }
}

Track::Type CueParser::matchTrackType(const std::string& s) const {
    if (s == "MODE2/2352") return Track::Type::DATA;
    if (s == "AUDIO") return Track::Type::AUDIO;

    throw std::runtime_error(string_format("Unsupported track type %s", s.c_str()));
}

std::unique_ptr<Cue> CueParser::parse(const char* path) {
    cuePath = getPath(path);
    auto contents = getFileContentsAsString(path);
    if (contents.empty()) return nullptr;

    globalOffset = Position(0, 2, 0);

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
}  // namespace utils
