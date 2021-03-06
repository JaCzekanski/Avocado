#include "load.h"
#include <array>
#include <disc/format/ecm_parser.h>
#include "disc/format/chd_format.h"
#include "disc/format/cue_parser.h"
#include "utils/file.h"

namespace disc {
const std::array<std::string, 6> discFormats = {"chd", "cue", "iso", "bin", "img", "ecm"};

bool isDiscImage(const std::string& path) {
    std::string ext = getExtension(path);
    transform(ext.begin(), ext.end(), ext.begin(), tolower);

    return std::find(discFormats.begin(), discFormats.end(), ext) != discFormats.end();
}

std::unique_ptr<disc::Disc> load(const std::string& path) {
    std::string ext = getExtension(path);
    transform(ext.begin(), ext.end(), ext.begin(), tolower);

    std::unique_ptr<disc::Disc> disc;
    if (ext == "chd") {
        disc = disc::format::Chd::open(path);
    } else if (ext == "cue") {
        disc::format::CueParser parser;
        disc = parser.parse(path.c_str());
    } else if (ext == "iso" || ext == "bin" || ext == "img") {
        disc = disc::format::Cue::fromBin(path.c_str());
    } else if (ext == "ecm") {
        disc::format::EcmParser parser;
        disc = parser.parse(path.c_str());
    }

    return disc;
}
}  // namespace disc