#include "card_formats.h"
#include "utils/file.h"
#include "fmt/core.h"
#include <algorithm>

namespace memory_card {
// Thanks ShendoXT - https://github.com/ShendoXT/memcardrex
const std::array<std::string, 9> rawFormats = {
    // Raw 128kB images
    "psm",  // SmartLink
    "ps",   // WinPSM
    "ddf",  // DataDeck
    "mcr",  // FPSX
    "mcd",  // ePSXe
    "mc",   // PSXGame Edit
    "mci",  // MCExplorer
    "srm",  // PCSX ReARMed/RetroArch
    "vm1",  // PS3 virtual card
};

const std::array<std::string, 4> complexFormats = {
    "mem",
    "vgs",  // Connectix Virtual Game Station
    "gme",  // DexDrive
    "vmp",  // PSP/Vita
};

const std::array<std::string, 8> saveFileFormats = {
    "mcs",  // PSXGameEdit single save
    "psv",  // PS3 signed save
    "psx",  // XP, AR, GS, Caetla single save
    "ps1",  // Memory Juggler
    "mcb",  // Smart Link
    "mcx",
    "pda",            // Datel
    "B???????????*",  // RAW single save
};

bool isMemoryCardImage(const std::string& path) {
    std::string ext = getExtension(path);
    transform(ext.begin(), ext.end(), ext.begin(), tolower);

    if (std::find(rawFormats.begin(), rawFormats.end(), ext) != rawFormats.end()) {
        return true;
    }

    if (std::find(complexFormats.begin(), complexFormats.end(), ext) != complexFormats.end()) {
        return true;
    }

    return false;
}

constexpr int VGS_HEADER_SIZE = 64;
constexpr int GME_HEADER_SIZE = 3904;
constexpr int VMP_HEADER_SIZE = 128;

std::optional<std::vector<uint8_t>> load(const std::string& path) {
    using namespace std::string_literals;
    std::string ext = getExtension(path);
    transform(ext.begin(), ext.end(), ext.begin(), tolower);

    auto raw = getFileContents(path);
    int offset = 0;

    auto slice = [](const std::vector<uint8_t>& vec, size_t len) {
        return std::string(vec.begin(), vec.begin() + (int)std::min(vec.size(), len));  //
    };

    if (ext == "raw" || ext == "ps" || ext == "ddf" || ext == "mcr" || ext == "mcd") {
        if (slice(raw, 2) != "MC"s) {
            fmt::print("[memory_card] Raw memory card image - Invalid header (expected MC).\n");
            return {};
        }
    } else if (ext == "mem" || ext == "vgs") {
        if (slice(raw, 4) != "VgsM"s) {
            fmt::print("[memory_card] VGS image - Invalid header (expected VgsM).\n");
            return {};
        }
        offset = VGS_HEADER_SIZE;
    } else if (ext == "gme") {
        if (slice(raw, 11) != "123-456-STD"s) {
            fmt::print("[memory_card] DexDrive image - Invalid header (expected 123-456-STD).\n");
            return {};
        }

        offset = GME_HEADER_SIZE;
    } else if (ext == "vmp") {
        if (slice(raw, 4) != "\0PMV"s) {
            fmt::print("[memory_card] PSP/Vita image - Invalid header (expected \\0PMV).\n");
            return {};
        }

        offset = VMP_HEADER_SIZE;
    } else {
        fmt::print("Unsupported memory card image format {}, please report it here https://github.com/JaCzekanski/Avocado/issues/new\n",
                   ext);
        return {};
    }

    std::vector<uint8_t> memory(MEMCARD_SIZE, 0);
    int size = std::min(MEMCARD_SIZE, (int)raw.size() - offset);
    std::copy(raw.begin() + offset, raw.begin() + offset + size, memory.begin());
    return memory;
}

bool saveRaw(const std::array<uint8_t, MEMCARD_SIZE>& data, const std::string& path) {
    auto output = std::vector<uint8_t>(data.begin(), data.end());

    if (!putFileContents(path, output)) {
        return false;
    }
    return true;
}

bool saveVgs(const std::array<uint8_t, MEMCARD_SIZE>& data, const std::string& path) {
    auto output = std::vector<uint8_t>(VGS_HEADER_SIZE + MEMCARD_SIZE);

    output[0] = 'V';
    output[1] = 'g';
    output[2] = 's';
    output[3] = 'M';

    output[4] = 1;
    output[8] = 1;
    output[12] = 1;
    output[17] = 2;

    std::copy(data.begin(), data.end(), output.begin() + VGS_HEADER_SIZE);

    if (!putFileContents(path, output)) {
        return false;
    }
    return true;
}

bool saveGme(const std::array<uint8_t, MEMCARD_SIZE>& data, const std::string& path) {
    auto output = std::vector<uint8_t>(GME_HEADER_SIZE + MEMCARD_SIZE);

    output[0] = '1';
    output[1] = '2';
    output[2] = '3';
    output[3] = '-';
    output[4] = '4';
    output[5] = '5';
    output[6] = '6';
    output[7] = '-';
    output[8] = 'S';
    output[9] = 'T';
    output[10] = 'D';

    output[18] = 0x1;
    output[20] = 0x1;
    output[21] = 'M';

    for (int slot = 0; slot < 15; slot++) {
        int ptr = (slot + 1) * 0x80;
        output[22 + slot] = data[ptr + 0];
        output[38 + slot] = data[ptr + 8];
    }

    // Comments are skipped

    std::copy(data.begin(), data.end(), output.begin() + GME_HEADER_SIZE);

    if (!putFileContents(path, output)) {
        return false;
    }
    return true;
}

bool saveVmp(const std::array<uint8_t, MEMCARD_SIZE>& data, const std::string& path) {
    auto output = std::vector<uint8_t>(VMP_HEADER_SIZE + MEMCARD_SIZE);

    output[0] = 0;
    output[1] = 'P';
    output[2] = 'M';
    output[3] = 'V';
    output[4] = 0x80;

    // offset: 0x0c, length: 0x14 - key seed, aes 128 cbc
    // offset: 0x20, length: 0x14 - sha1 hmac

    std::copy(data.begin(), data.end(), output.begin() + VMP_HEADER_SIZE);

    if (!putFileContents(path, output)) {
        return false;
    }
    return true;
}

bool save(const std::array<uint8_t, MEMCARD_SIZE>& data, const std::string& path) {
    std::string ext = getExtension(path);
    transform(ext.begin(), ext.end(), ext.begin(), tolower);

    if (ext == "raw" || ext == "ps" || ext == "ddf" || ext == "mcr" || ext == "mcd") {
        return saveRaw(data, path);
    } else if (ext == "mem" || ext == "vgs") {
        return saveVgs(data, path);
    } else if (ext == "gme") {
        return saveGme(data, path);
    } else if (ext == "vmp") {
        return saveVmp(data, path);
    } else {
        fmt::print("Unsupported memory card image format {}, please report it here https://github.com/JaCzekanski/Avocado/issues/new\n",
                   ext);
        return false;
    }
}

void format(std::array<uint8_t, MEMCARD_SIZE>& data) {
    std::array<uint8_t, 0x80> frame;

    auto writeFrame = [&](int frameNum, bool withChecksum = false) {
        if (withChecksum) {
            uint8_t checksum = 0;
            for (int i = 0; i < 0x7f; i++) {
                checksum ^= frame[i];
            }
            frame[0x7f] = checksum;
        }

        int offset = frameNum * 128;
        for (int i = 0; i < 0x80; i++) {
            data[offset + i] = frame[i];
        }
    };

    data.fill(0);

    frame.fill(0);
    frame[0] = 'M';
    frame[1] = 'C';
    writeFrame(0, true);
    writeFrame(63, true);

    // Directory frames
    for (int i = 0; i < 15; i++) {
        frame.fill(0);
        // 0x00-0x03 - Block allocation state
        frame[0] = 0xa0;  // Free, freshly formatted
        frame[1] = 0x00;
        frame[2] = 0x00;
        frame[3] = 0x00;

        // Pointer to next block
        frame[8] = 0xff;
        frame[9] = 0xff;
        writeFrame(1 + i, true);
    }

    // Broken sector list
    for (int i = 0; i < 20; i++) {
        frame.fill(0);
        // Sector number
        frame[0] = 0xff;
        frame[1] = 0xff;
        frame[2] = 0xff;
        frame[3] = 0xff;

        // Garbage
        frame[8] = 0xff;
        frame[9] = 0xff;
        writeFrame(16 + i);
    }
}
}  // namespace memory_card