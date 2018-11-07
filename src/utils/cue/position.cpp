#include "position.h"
#include "utils/string.h"

namespace utils {
Position::Position() : mm(0), ss(0), ff(0) {}

Position::Position(int mm, int ss, int ff) : mm(mm), ss(ss), ff(ff) {}

Position Position::fromLba(size_t lba) {
    int mm = (int)lba / 60 / 75;
    int ss = ((int)lba % (60 * 75)) / 75;
    int ff = (int)lba % 75;
    return Position(mm, ss, ff);
}

std::string Position::toString() const { return string_format("%02d:%02d:%02d", mm, ss, ff); }

int Position::toLba() const { return (mm * 60 * 75) + (ss * 75) + ff; }

Position Position::operator+(const Position& p) const { return fromLba(toLba() + p.toLba()); }

Position Position::operator-(const Position& p) const { return fromLba(toLba() - p.toLba()); }

bool Position::operator==(const Position& p) const { return toLba() == p.toLba(); }

bool Position::operator>=(const Position& p) const { return toLba() >= p.toLba(); }

bool Position::operator<(const Position& p) const { return toLba() < p.toLba(); }
}  // namespace utils
