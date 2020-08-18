#include "wave.h"
#include <cstring>
#include <cstdio> // for FILE, fwrite, fclose

namespace wave {
bool writeToFile(const std::vector<uint16_t>& buffer, const char* filename, int channels) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        return false;
    }
    auto wstr = [&](const char* str) { fwrite(str, 1, strlen(str), f); };
    auto w32 = [f](uint32_t i) { fwrite(&i, sizeof(i), 1, f); };
    auto w16 = [f](uint16_t i) { fwrite(&i, sizeof(i), 1, f); };

    const int bitPerSample = 16;
    const int sampleRate = 44100;

    wstr("RIFF");
    w32((buffer.size() * bitPerSample / 8) + 36);

    wstr("WAVE");
    wstr("fmt ");
    w32(16);  // Subchunk size
    w16(1);   // PCM
    w16(channels);
    w32(sampleRate);
    w32(sampleRate * channels * bitPerSample / 8);
    w16(channels * bitPerSample / 8);
    w16(bitPerSample);

    wstr("data");
    w32(buffer.size() * bitPerSample / 8);
    for (uint16_t s : buffer) {
        w16(s);
    }

    fclose(f);
    return true;
}
};  // namespace wave