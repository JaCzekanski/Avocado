#pragma once
#include "utils/cue/cue.h"

namespace AudioCD {
extern utils::Position currentPosition;
void init();
void play(utils::Cue& cue, utils::Position& position);
void stop();
void close();
};  // namespace AudioCD