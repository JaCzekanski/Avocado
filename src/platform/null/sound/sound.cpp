#include "sound/sound.h"

namespace Sound {
std::deque<uint16_t> buffer;
std::mutex audioMutex;
}  // namespace Sound

void Sound::init() {}

void Sound::play() {}

void Sound::stop() {}

void Sound::close() {}
