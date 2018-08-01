#include "audio_cd.h"
#include <algorithm>
#include <deque>
#include "sound.h"

using namespace utils;

Position AudioCD::currentPosition;

extern std::deque<int16_t> audioBuf;

namespace {
Cue currentCue;

void audioCallback(uint8_t* raw_stream, int len) {
    auto bytes = currentCue.read(AudioCD::currentPosition, len, true);
    AudioCD::currentPosition = AudioCD::currentPosition + Position(0, 0, 4);
    uint8_t* samples = (uint8_t*)bytes.data();

    for (int i = 0; i < len; i += 2) {
        if (audioBuf.empty()) break;
        auto sample = audioBuf.front();
        audioBuf.pop_front();

        raw_stream[i] = sample;
        raw_stream[i + 1] = sample >> 8;
    }
    printf("Audio: consumed %d bytes\n", len);
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
