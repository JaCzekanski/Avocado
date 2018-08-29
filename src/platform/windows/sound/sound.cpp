#include "sound/sound.h"
#include <SDL.h>

namespace Sound {
std::deque<uint16_t> buffer;
std::mutex audioMutex;
};  // namespace Sound

namespace {
SDL_AudioDeviceID dev = 0;

void audioCallback(void* userdata, Uint8* raw_stream, int len) {
    for (int i = 0; i < len; i++) raw_stream[i] = 0;

    std::unique_lock<std::mutex> lock(Sound::audioMutex);
    size_t bufSize = Sound::buffer.size();
    for (size_t i = 0; i < (size_t)len; i += 2) {
        if (i / 2 >= bufSize) break;

        int16_t sample = Sound::buffer.front();
        Sound::buffer.pop_front();
        raw_stream[i] = sample & 0xff;
        raw_stream[i + 1] = (sample >> 8) & 0xff;
    }
}
}  // namespace

void Sound::init() {
    SDL_AudioSpec desired = {}, obtained;
    desired.freq = 44100;
    desired.format = AUDIO_S16;
    desired.channels = 2;
    desired.samples = 512;
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

void Sound::play() { SDL_PauseAudioDevice(dev, false); }

void Sound::stop() { SDL_PauseAudioDevice(dev, true); }

void Sound::close() { SDL_CloseAudioDevice(dev); }

void Sound::clearBuffer() { buffer.clear(); }
