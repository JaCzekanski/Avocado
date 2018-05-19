#include "sound/sound.h"
#include <SDL.h>
#include "utils/cue/track.h"

namespace {
SDL_AudioDeviceID dev = 0;
Sound::AudioCallback _callback = nullptr;

void audioCallback(void* userdata, Uint8* raw_stream, int len) {
    for (int i = 0; i < len; i++) {
        raw_stream[i] = 0;
    }

    if (_callback != nullptr) _callback(raw_stream, len);
}
}  // namespace

void Sound::init(AudioCallback callback) {
    SDL_AudioSpec desired = {}, obtained;
    desired.freq = 44100;
    desired.format = AUDIO_S16;
    desired.channels = 2;
    desired.samples = utils::Track::SECTOR_SIZE;
    desired.callback = audioCallback;

    _callback = callback;

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

void Sound::play() { SDL_PauseAudioDevice(dev, false); }

void Sound::stop() { SDL_PauseAudioDevice(dev, true); }

void Sound::close() { SDL_CloseAudioDevice(dev); }
