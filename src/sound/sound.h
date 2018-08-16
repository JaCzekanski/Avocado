#pragma once
#include <cstdint>
#include <iterator>
#include <deque>
#include <mutex>

namespace Sound {
extern std::deque<uint16_t> buffer;
extern std::mutex audioMutex;

void init();
void play();
void stop();
void close();
void clearBuffer();

template<typename Iterator>
void appendBuffer( const Iterator& start, const Iterator& end ) {
    std::unique_lock<std::mutex> lock(audioMutex);
    buffer.insert(buffer.end(), start, end);
}
};  // namespace Sound
