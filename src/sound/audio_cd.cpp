#include "audio_cd.h"
#include "sound.h"

using namespace utils;

Position AudioCD::currentPosition;

namespace {
Cue currentCue;

void audioCallback(uint8_t* raw_stream, int len) {
    auto bytes = currentCue.read(AudioCD::currentPosition, len, true);
    AudioCD::currentPosition = AudioCD::currentPosition + Position(0, 0, 4);
    uint8_t* samples = (uint8_t*)bytes.data();

    for (int i = 0; i < bytes.size(); i++) {
        raw_stream[i] = samples[i];
    }
}
}  // namespace

void AudioCD::init() { Sound::init(audioCallback); }

void AudioCD::play(Cue& cue, Position& position) {
    currentCue = cue;
    currentPosition = position;

    Sound::play();
}

void AudioCD::stop() { Sound::stop(); }

void AudioCD::close() { Sound::close(); }
