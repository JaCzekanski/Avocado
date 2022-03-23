#pragma once
#include <functional>
#include <string>

namespace disc {
struct Position {
    int mm;
    int ss;
    int ff;

    Position();
    Position(int mm, int ss, int ff);
    static Position fromLba(size_t lba);

    std::string toString() const;
    int toLba() const;
    Position operator+(const Position& p) const;
    Position operator-(const Position& p) const;
    Position& operator+=(const Position& p);
    Position& operator-=(const Position& p);
    bool operator==(const Position& p) const;
    bool operator>=(const Position& p) const;
    bool operator<(const Position& p) const;
};
}  // namespace disc

namespace std {
template <>
struct hash<disc::Position> {
    std::size_t operator()(const disc::Position& p) const {
        return ((p.mm ^ (p.ss << 1)) >> 1) ^ (p.ff << 1);  //
    }
};
}  // namespace std