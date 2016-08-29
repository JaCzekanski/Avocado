#include "dummy.h"

namespace device {
Dummy::Dummy() : name("") {}
Dummy::Dummy(std::string name, uint32_t baseAddress, bool verbose) : name(name), baseAddress(baseAddress), verbose(verbose) {}

void Dummy::step() {}

uint8_t Dummy::read(uint32_t address) {
    if (verbose) printf("%-10s R 0x%08x\n", name.c_str(), address + baseAddress);
    return 0;
}

void Dummy::write(uint32_t address, uint8_t data) {
    if (verbose) printf("%-10s W 0x%08x - 0x%02x\n", name.c_str(), address + baseAddress, data);
}
}