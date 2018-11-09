#include "cue_parser.h"
#include <sstream>
#include "utils/file.h"
#include "utils/string.h"

namespace utils {

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
        throw ParsingException(string_format("Unsupported file type %s", lastFileType.c_str()));
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
        throw ParsingException(string_format("Cue with index > 1 (%d given) are not supported", index));
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

    // if (track.number == 1 && track.type == Track::Type::DATA) {
    //     track.pregap = track.pregap + Position{0,2,0};
    // }

    if (track.pregap == Position{0, 0, 0} && track.index0) {
        track.pregap = track.index1 - *track.index0;
    } else {
        track.index0 = track.index1;
    }

    if (!track.index0) {
        track.index0 = track.index1 - track.pregap;
    }

    cue.tracks.push_back(track);
    track = Track();
}

void CueParser::fixTracksLength() {
    for (int t = 0; t < cue.tracks.size(); t++) {
        // Last track / single track .cue
        if (t == cue.tracks.size() - 1) {
            size_t size = getFileSize(cue.tracks[t].filename);
            if (size == 0) {
                throw ParsingException(string_format("File %s not found", cue.tracks[t].filename.c_str()));
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
                    throw ParsingException(string_format("Something's fucky, track %d has no frames.", t));
                }
            } else {
                size_t size = getFileSize(cue.tracks[t].filename);
                if (size == 0) {
                    throw ParsingException(string_format("File %s not found", cue.tracks[t].filename.c_str()));
                }

                cue.tracks[t].offset = 0;
                cue.tracks[t].frames = size / Track::SECTOR_SIZE;  // TODO: SECTOR_SIZE to getFrameSize
            }
        }
    }
}

Track::Type CueParser::matchTrackType(const std::string& s) const {
    if (s == "MODE2/2352") return Track::Type::DATA;
    if (s == "AUDIO") return Track::Type::AUDIO;

    throw ParsingException(string_format("Unsupported track type %s", s.c_str()));
}

std::optional<Cue> CueParser::parse(const char* path) {
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
            printf("[CUE] Unparsed line: %s\n", line.c_str());
        }

        addTrackToCue();  // ?
        fixTracksLength();
    } catch (ParsingException& e) {
        printf("[DISK] Error loading .cue: %s\n", e.what());
        return {};
    }

    if (cue.getTrackCount() == 0) return {};
    return cue;
}
}  // namespace utils
