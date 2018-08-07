#include "psf.h"
#include "miniz.h"
#include <cstring>
#include <sstream>

uint32_t read_u32(const std::vector<uint8_t>& vec, size_t offset) {
    if (offset+4 > vec.size()) return 0;

    uint32_t ret = 0;
    ret |= vec[offset];
    ret |= vec[offset+1] << 8;
    ret |= vec[offset+2] << 16;
    ret |= vec[offset+3] << 24;
    return ret;
}

bool loadPsf(System* sys, const std::string& path) {
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
    auto compressedSize = read_u32(file, 8);

    std::vector<uint8_t> exe;
    size_t exeSize = 2*1024*1024;
    exe.resize(exeSize);
    
    mz_uncompress(exe.data(), &exeSize, file.data()+16, compressedSize);

    exe.resize(exeSize);

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
            if (line.rfind("_lib", 0) == 0) {
                // Load libs
            }
            
            printf("[PSF]%s\n", line.c_str());
        }
    }

putFileContents("dump.exe" ,exe);
    sys->loadExeFile(exe);

    return true;
}