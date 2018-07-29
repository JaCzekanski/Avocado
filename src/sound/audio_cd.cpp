#include "audio_cd.h"
#include <algorithm>
#include "sound.h"

using namespace utils;

Position AudioCD::currentPosition;

extern std::vector<int16_t> audioBuf;

namespace {
Cue currentCue;

void audioCallback(uint8_t* raw_stream, int len) {
    auto bytes = currentCue.read(AudioCD::currentPosition, len, true);
    AudioCD::currentPosition = AudioCD::currentPosition + Position(0, 0, 4);
    uint8_t* samples = (uint8_t*)bytes.data();

    int length = std::min<int>(len, audioBuf.size());
    for (int i = 0; i < length; i++) {
        raw_stream[i] = audioBuf[i];
    }
    if (audioBuf.size() >= length) {
        audioBuf.erase(audioBuf.begin(), audioBuf.begin() + length);
    }
}
}  // namespace

void AudioCD::init() { Sound::init(audioCallback); }

void AudioCD::play(Cue& cue, Position& position) {
    currentCue = cue;
    currentPosition = position;

    Sound::play();
}

void AudioCD::stop() { /* Sound::stop();*/
}

void AudioCD::close() { Sound::close(); }
