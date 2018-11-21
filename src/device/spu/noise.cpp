#include "noise.h"
#include <array>

namespace {
std::array<int8_t, 64> noise_addition = {
	{1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0,
	 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0,
	 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1,
	 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1}};

std::array<int8_t, 5> noise_halfcycle = {{0, 84, 140, 180, 210}};
};

namespace spu {
Noise::Noise() {
	noiseFrequency = 0;
	noiseLevel = 0;
}

int16_t Noise::getNoiseLevel() {
	return noiseLevel;
}

void Noise::doNoise(uint16_t step, uint16_t shift) {
	int freq = (0x8000 >> shift) << 16;

	noiseFrequency += 0x10000;
	noiseFrequency += noise_halfcycle[step];

	if ((noiseFrequency & 0xffff) >= noise_halfcycle[4]) {
		noiseFrequency += 0x10000;
		noiseFrequency -= noise_halfcycle[step];
	}

	if (noiseFrequency >= freq) {
		noiseFrequency %= freq;

		int index = (noiseLevel >> 10) & 0x3f;
		noiseLevel = (noiseLevel << 1) + noise_addition[index];
	}
}
}  // namespace spu