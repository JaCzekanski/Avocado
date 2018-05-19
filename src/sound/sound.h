#pragma once
#include <cstdint>

namespace Sound {
typedef void (*AudioCallback)(uint8_t *stream, int len);

void init(AudioCallback callback);
void play();
void stop();
void close();
};  // namespace Sound