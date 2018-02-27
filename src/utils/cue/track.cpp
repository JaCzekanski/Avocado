#include "track.h"

namespace utils {
Position Track::getTrackSize() const { return end - (start + pause + pregap); }
}  // namespace utils
