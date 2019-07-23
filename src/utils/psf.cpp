#include "psf.h"
#include <cstring>
#include <sstream>
#include "miniz.h"
#include "utils/file.h"
#include "utils/psx_exe.h"

namespace {
uint32_t read_u32(const std::vector<uint8_t>& vec, size_t offset) {
    if (offset + 4 > vec.size()) return 0;

    uint32_t ret = 0;
    ret |= vec[offset];
    ret |= vec[offset + 1] << 8;
    ret |= vec[offset + 2] << 16;
    ret |= vec[offset + 3] << 24;
    return ret;
}

bool loadExe(System* sys, const std::vector<uint8_t>& _exe, PsfType type) {
    if (_exe.empty()) return false;
    assert(_exe.size() >= 0x800);

    PsxExe exe;
    memcpy(&exe, _exe.data(), sizeof(exe));

    if (exe.t_size > _exe.size() - 0x800) {
        printf("Invalid exe t_size: 0x%08x\n", exe.t_size);
        return false;
    }

    for (uint32_t i = 0; i < exe.t_size; i++) {
        sys->writeMemory8(exe.t_addr + i, _exe[0x800 + i]);
    }

    if (type == PsfType::Main || type == PsfType::MainLib) {
        for (int i = 0; i < 32; i++) sys->cpu->reg[i] = 0;
        sys->cpu->setPC(exe.pc0);
        sys->cpu->reg[28] = exe.gp0;
        sys->cpu->reg[29] = exe.s_addr;
        if (sys->cpu->reg[29] == 0) sys->cpu->reg[29] = 0x801ffff0;

        sys->cpu->exception = false;
        sys->cpu->inBranchDelay = false;
    }

    return true;
}
}  // namespace

bool loadPsf(System* sys, const std::string& path, PsfType type) {
    printf("Loading %s\n", path.c_str());
    std::string ext = getExtension(path);
    transform(ext.begin(), ext.end(), ext.begin(), tolower);

    if (ext != "psf" && ext != "minipsf" && ext != "psflib") {
        printf("[PSF] .psf and .minipsf extensions are supported.\n");
        return false;
    }

    auto file = getFileContents(path);
    if (file.empty()) {
        return false;
    }

    if (file.size() < 16 || memcmp("PSF\x01", file.data(), 4) != 0) {
        printf("[PSF] Invalid header\n");
        return false;
    }

    auto reservedArea = read_u32(file, 4);
    if (reservedArea != 0) {
        printf("[PSF] Reserved area != 0\n");
    }
    auto compressedSize = read_u32(file, 8);

    std::vector<uint8_t> exe;
    mz_ulong exeSize = 2 * 1024 * 1024;
    exe.resize(exeSize);

    mz_uncompress(exe.data(), &exeSize, file.data() + 16, compressedSize);

    exe.resize(exeSize);
    loadExe(sys, exe, type);

    bool libsLoaded = false;
    auto tagOffset = 16 + compressedSize;
    if (file.size() < tagOffset + 5 || memcmp("[TAG]", file.data() + tagOffset, 5) != 0) {
        return true;
    } else {
        std::string tags;
        tags.assign(file.begin() + tagOffset + 5, file.end());

        std::stringstream stream;
        stream.str(tags);

        std::string line;
        while (std::getline(stream, line)) {
            if (line.rfind("_lib=", 0) == 0) {
                std::string libFile = line.substr(line.find("=") + 1);
                std::string libPath = getPath(path) + libFile;

                loadPsf(sys, libPath, PsfType::MainLib);
                libsLoaded = true;
                continue;
            }
            if (line.rfind("_lib", 0) == 0) {
                std::string libFile = line.substr(line.find("=") + 1);
                std::string libPath = getPath(path) + libFile;

                loadPsf(sys, libPath, PsfType::SecondaryLib);
                libsLoaded = true;
                continue;
            }
            if (type != PsfType::Main) break;

            auto pos = line.find("=");
            if (pos == std::string::npos) continue;

            auto key = line.substr(0, pos);
            auto value = line.substr(pos + 1);
            printf("%s: %s\n", key.c_str(), value.c_str());
        }
    }

    if (libsLoaded && (type == PsfType::Main || type == PsfType::SecondaryLib)) {
        loadExe(sys, exe, PsfType::SecondaryLib);
    }

    return true;
}