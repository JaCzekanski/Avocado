#include "audio_cd.h"
#include <SDL.h>
#include <SDL_audio.h>

using namespace utils;

Position AudioCD::currentPosition;

namespace {
SDL_AudioDeviceID dev = 0;
Cue currentCue;

void audioCallback(void* userdata, Uint8* raw_stream, int len) {
    auto bytes = currentCue.read(AudioCD::currentPosition, len, true);
    AudioCD::currentPosition = AudioCD::currentPosition + Position(0, 0, 4);
    uint8_t* samples = (uint8_t*)bytes.data();

    for (int i = 0; i < len; i++) {
        raw_stream[i] = 0;
    }

    for (int i = 0; i < bytes.size(); i++) {
        raw_stream[i] = samples[i];
    }

    return;
}
}  // namespace

void AudioCD::init() {
    SDL_AudioSpec desired = {}, obtained;
    desired.freq = 44100;
    desired.format = AUDIO_S16;
    desired.channels = 2;
    desired.samples = Track::SECTOR_SIZE;
    desired.callback = audioCallback;

    dev = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);

    if (dev == 0) {
        printf("SDL_OpenAudioDevice error: %s\n", SDL_GetError());
        return;
    }

    if (obtained.freq != desired.freq || obtained.format != desired.format || obtained.channels != desired.channels
        || obtained.samples != desired.samples) {
        printf("SDL_OpenAudio obtained audio spec is different from desired, audio might sound wrong.\n");
        return;
    }
}

void AudioCD::play(Cue& cue, Position& position) {
    currentCue = cue;
    currentPosition = position;

    SDL_PauseAudioDevice(dev, false);
}

void AudioCD::stop() { SDL_PauseAudioDevice(dev, true); }

void AudioCD::close() { SDL_CloseAudioDevice(dev); }
