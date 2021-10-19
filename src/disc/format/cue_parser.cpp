#include "cue_parser.h"
#include <fmt/core.h>
#include <sstream>
#include "utils/file.h"

namespace disc {
namespace format {
bool CueParser::parseFile(std::string& line) {
    std::smatch matches;
    regex_search(line, matches, regexFile);
    if (matches.size() != 3) {
        regex_search(line, matches, regexFile2);
        if (matches.size() != 3) return false;
    }

    lastFile = cuePath + "/" + matches[1].str();
    lastFileType = matches[2].str();

    // TODO: Handle WAVE type
    if (lastFileType != "BINARY") {
        throw ParsingException(fmt::format("Unsupported file type {}", lastFileType));
    }
    return true;
}

bool CueParser::parseTrack(std::string& line) {
    std::smatch matches;
    regex_search(line, matches, regexTrack);
    if (matches.size() != 3) return false;

    track.number = stoi(matches[1].str());
    track.type = matchTrackType(matches[2].str());
    track.filename = lastFile;
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
        track.index0 = {mm, ss, ff};
    } else if (index == 1) {
        track.index1 = {mm, ss, ff};
        addTrackToCue();
    } else {
        throw ParsingException(fmt::format("Cue with index > 1 ({} given) are not supported", index));
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

bool CueParser::parsePostgap(std::string& line) {
    std::smatch matches;
    regex_search(line, matches, regexPostgap);
    if (matches.size() != 4) return false;

    int mm = stoi(matches[1].str());
    int ss = stoi(matches[2].str());
    int ff = stoi(matches[3].str());
    track.postgap = {mm, ss, ff};
    return true;
}

void CueParser::addTrackToCue() {
    // ignore if track is not completed
    if (track.number == 0) return;

    if (!track.index0) {
        track.index0 = track.index1;
    }

    cue.tracks.push_back(track);
    track = Track();
}

void CueParser::fixTracksLength() {
    for (unsigned t = 0; t < cue.tracks.size(); t++) {
        // Last track / single track .cue
        if (t == cue.tracks.size() - 1) {
            size_t size = getFileSize(cue.tracks[t].filename);
            if (size == 0) {
                throw ParsingException(fmt::format("File {} not found", cue.tracks[t].filename));
            }

            if (t != 0 && cue.tracks[t].filename == cue.tracks[t - 1].filename) {
                cue.tracks[t].offset = cue.tracks[t - 1].offset + cue.tracks[t - 1].frames * Track::SECTOR_SIZE;
                cue.tracks[t].frames = (size - cue.tracks[t].offset) / Track::SECTOR_SIZE;
            } else {
                cue.tracks[t].offset = 0;
                cue.tracks[t].frames = size / Track::SECTOR_SIZE;  // TODO: SECTOR_SIZE to getFrameSize
            }
        } else {
            if (cue.tracks[t].filename == cue.tracks[t + 1].filename) {
                cue.tracks[t].frames = (*(cue.tracks[t + 1].index0) - *(cue.tracks[t].index0)).toLba();

                if (t == 0) {
                    cue.tracks[t].offset = 0;
                } else {
                    cue.tracks[t].offset = cue.tracks[t - 1].offset + cue.tracks[t - 1].frames * Track::SECTOR_SIZE;
                }

                if (cue.tracks[t].frames == 0) {
                    throw ParsingException(fmt::format("Something's fucky, track {} has no frames.", t));
                }
            } else {
                size_t size = getFileSize(cue.tracks[t].filename);
                if (size == 0) {
                    throw ParsingException(fmt::format("File {} not found", cue.tracks[t].filename));
                }

                cue.tracks[t].offset = 0;
                cue.tracks[t].frames = size / Track::SECTOR_SIZE;  // TODO: SECTOR_SIZE to getFrameSize
            }
        }
    }
}

disc::TrackType CueParser::matchTrackType(const std::string& s) const {
    if (s == "MODE2/2352") return disc::TrackType::DATA;
    if (s == "AUDIO") return disc::TrackType::AUDIO;

    throw ParsingException(fmt::format("Unsupported track type {}", s));
}

std::unique_ptr<Cue> CueParser::parse(const char* path) {
    cuePath = getPath(path);
    auto contents = getFileContentsAsString(path);
    if (contents.empty()) {
        return {};
    }

    cue.file = path;

    try {
        std::stringstream stream;
        stream.str(contents);

        std::string line;
        while (std::getline(stream, line)) {
            if (parseFile(line)) continue;
            if (parseTrack(line)) continue;
            if (parseIndex(line)) continue;
            if (parsePregap(line)) continue;
            if (parsePostgap(line)) continue;
            fmt::print("[CUE] Unparsed line: {}\n", line);
        }

        addTrackToCue();  // ?
        fixTracksLength();
    } catch (ParsingException& e) {
        fmt::print("[DISK] Error loading .cue: {}\n", e.what());
        return {};
    }

    if (cue.getTrackCount() == 0) return {};

    cue.loadSubchannel(path);

    return std::make_unique<Cue>(cue);
}
};  // namespace format
}  // namespace disc
