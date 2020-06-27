#include "ecm_parser.h"
#include <fmt/core.h>
#include <utility>
#include <array>

namespace {
constexpr std::array<uint32_t, 256> edcLUT = []() {
    std::array<uint32_t, 256> lut = {};
    for (int i = 0; i < 256; i++) {
        uint32_t edc = i;

        for (int j = 0; j < 8; j++) {
            bool carry = edc & 1;
            edc = (edc >> 1) ^ (carry ? 0xD8018001 : 0);
        }

        lut[i] = edc;
    }
    return lut;
}();

constexpr uint32_t eccEntry(uint8_t i) { return (i << 1) ^ (i & 0x80 ? 0x11d : 0); }

constexpr std::array<uint8_t, 256> eccfLUT = []() {
    std::array<uint8_t, 256> lut = {};
    for (int i = 0; i < 256; i++) {
        lut[i] = eccEntry(i);
    }
    return lut;
}();

constexpr std::array<uint8_t, 256> eccbLUT = []() {
    std::array<uint8_t, 256> lut = {};
    for (int i = 0; i < 256; i++) {
        lut[i ^ eccEntry(i)] = i;
    }
    return lut;
}();
}  // namespace

namespace disc::format {

uint32_t EcmParser::calculateEDC(size_t addr, size_t size) const {
    uint32_t edc = 0;
    for (size_t i = 0; i < size; i++) {
        edc ^= frame[addr + i];
        edc = (edc >> 8) ^ edcLUT[edc & 0xff];
    }
    return edc;
}

void EcmParser::adjustEDC(size_t addr, size_t size) {
    uint32_t edc = calculateEDC(addr, size);
    size_t offset = addr + size;

    for (int i = 0; i < 4; i++) {
        frame[offset + i] = (edc >> (i * 8)) & 0xff;
    }
}

void EcmParser::adjustSync() {
    frame[0] = 0;
    for (int i = 1; i < 11; i++) frame[i] = 0xff;
    frame[11] = 0;
}

void EcmParser::copySubheader() {
    for (int i = 0x10; i < 0x14; i++) {
        frame[i] = frame[i + 4];
    }
}

void EcmParser::computeECCblock(uint8_t* src, uint32_t majorCount, uint32_t minorCount, uint32_t majorMult, uint32_t minorInc,
                                uint8_t* dst) {
    uint32_t size = majorCount * minorCount;
    for (uint32_t major = 0; major < majorCount; major++) {
        uint32_t index = (major >> 1) * majorMult + (major & 1);
        uint32_t a = 0, b = 0;

        for (uint32_t minor = 0; minor < minorCount; minor++) {
            uint8_t temp = src[index];
            index += minorInc;
            if (index >= size) index -= size;
            a ^= temp;
            b ^= temp;

            a = eccfLUT[a];
        }
        a = eccbLUT[eccfLUT[a] ^ b];
        dst[major] = a;
        dst[major + majorCount] = a ^ b;
    }
}

void EcmParser::calculateAndAdjustECC() {
    computeECCblock(frame + 0xc, 86, 24, 2, 86, frame + 0x81c);
    computeECCblock(frame + 0x0c, 52, 43, 86, 88, frame + 0x8c8);
}

void EcmParser::handleMode1() {
    fread(frame + 0x0c, 1, 0x804, f.get());

    adjustSync();
    frame[0xf] = 0x01;

    adjustEDC(0x00, 0x810);
    for (int i = 0x814; i < 0x818; i++) frame[i] = 0;

    calculateAndAdjustECC();

    data.insert(data.end(), frame, frame + 2352);
}

void EcmParser::handleMode2Form1() {
    fread(frame + 0x14, 1, 0x804, f.get());

    adjustSync();
    frame[0xf] = 0x02;
    copySubheader();

    adjustEDC(0x10, 0x808);

    uint8_t _address[4];
    for (int i = 0; i < 4; i++) {
        _address[i] = frame[12 + i];
        frame[12 + i] = 0;
    }

    calculateAndAdjustECC();

    for (int i = 0; i < 4; i++) {
        frame[12 + i] = _address[i];
    }

    data.insert(data.end(), frame + 0x10, frame + 0x10 + 2336);
}

void EcmParser::handleMode2Form2() {
    fread(frame + 0x14, 1, 0x918, f.get());

    adjustSync();
    frame[0xf] = 0x02;
    copySubheader();

    adjustEDC(0x10, 0x91c);
    // Mode2Form2 has no ECC

    data.insert(data.end(), frame + 0x10, frame + 0x10 + 2336);
}
std::unique_ptr<Ecm> EcmParser::parse(const char* file) {
    f = unique_ptr_file(fopen(file, "rb"));
    if (!f) {
        fmt::print("[ECM] Cannot open {}.\n", file);
        return {};
    }

    // Check header
    char header[4];
    fread(header, 1, 4, f.get());

    if (memcmp("ECM\0", header, 4) != 0) {
        fmt::print("[ECM] Invalid header.\n");
        return {};
    }

    data.clear();
    data.reserve(disc::Track::SECTOR_SIZE * 300000);

    for (;;) {
        int type = 0;
        uint32_t count = 0;

        for (int i = 0; i < 5; i++) {
            uint8_t byte = fgetc(f.get());

            if (i == 0) {
                type = byte & 0b11;

                count |= (byte & 0b0111'1100) >> 2;
            } else {
                int shift = 5 + (i - 1) * 7;
                count |= (byte & 0x7f) << shift;
            }

            if ((byte & 0x80) == 0) break;
        }

        if (count == 0xffff'ffff) {
            break;
        }

        count += 1;

        uint32_t sector = data.size() / Track::SECTOR_SIZE;

        if (count > 0x8000'0000) {
            // Corrupt file
            fmt::print("[ECM] Sector {} invalid count.\n", sector);
            return {};
        }

        if (type == 0) {
            while (count > 0) {
                int len = std::min<int>(Track::SECTOR_SIZE, count);
                fread(frame, 1, len, f.get());

                data.insert(data.end(), frame, frame + len);
                count -= len;
            }
        } else if (type == 1) {
            for (; count > 0; count--) handleMode1();
        } else if (type == 2) {
            for (; count > 0; count--) handleMode2Form1();
        } else if (type == 3) {
            for (; count > 0; count--) handleMode2Form2();
        } else {
            fmt::print("[ECM] Sector {}, invalid type ({}, expected 0..3)\n", sector, type);
            return {};
        }
    }

    data.shrink_to_fit();
    return std::make_unique<Ecm>(file, std::move(data));
}

}  // namespace disc::format