#include "reverb.h"
#include <vector>
#include "adsr.h"
#include "device/device.h"
#include "utils/math.h"

namespace spu {
uint32_t wrap(SPU* spu, uint32_t address) {
    const uint32_t reverbBase = spu->reverbBase._reg * 8;

    uint32_t rel = address - reverbBase;
    rel = rel % (spu->RAM_SIZE - reverbBase);

    return (reverbBase + rel) & 0x7fffe;
}

void write(SPU* spu, uint32_t address, float sample) {
    uint16_t clamped = floatToInt(clamp(sample, -1.f, 1.f));

    uint32_t addr = wrap(spu, spu->reverbCurrentAddress + address);
    spu->ram[addr + 0] = clamped & 0xff;
    spu->ram[addr + 1] = (clamped >> 8) & 0xff;
}

float read(SPU* spu, uint32_t address) {
    uint32_t addr = wrap(spu, spu->reverbCurrentAddress + address);
    uint16_t data = spu->ram[addr] | (spu->ram[addr + 1] << 8);
    return intToFloat(data);
}

std::tuple<int16_t, int16_t> doReverb(SPU* spu, std::tuple<int16_t, int16_t> input) {
    const uint32_t dAPF1 = spu->reverbRegisters[0x00]._reg * 8;
    const uint32_t dAPF2 = spu->reverbRegisters[0x01]._reg * 8;

    const float vIIR = intToFloat(spu->reverbRegisters[0x02]._reg);
    const float vCOMB1 = intToFloat(spu->reverbRegisters[0x03]._reg);
    const float vCOMB2 = intToFloat(spu->reverbRegisters[0x04]._reg);
    const float vCOMB3 = intToFloat(spu->reverbRegisters[0x05]._reg);
    const float vCOMB4 = intToFloat(spu->reverbRegisters[0x06]._reg);
    const float vWALL = intToFloat(spu->reverbRegisters[0x07]._reg);
    const float vAPF1 = intToFloat(spu->reverbRegisters[0x08]._reg);
    const float vAPF2 = intToFloat(spu->reverbRegisters[0x09]._reg);

    const uint32_t mLSAME = spu->reverbRegisters[0x0A]._reg * 8;
    const uint32_t mRSAME = spu->reverbRegisters[0x0B]._reg * 8;
    const uint32_t mLCOMB1 = spu->reverbRegisters[0x0C]._reg * 8;
    const uint32_t mRCOMB1 = spu->reverbRegisters[0x0D]._reg * 8;
    const uint32_t mLCOMB2 = spu->reverbRegisters[0x0E]._reg * 8;
    const uint32_t mRCOMB2 = spu->reverbRegisters[0x0F]._reg * 8;
    const uint32_t dLSAME = spu->reverbRegisters[0x10]._reg * 8;
    const uint32_t dRSAME = spu->reverbRegisters[0x11]._reg * 8;
    const uint32_t mLDIFF = spu->reverbRegisters[0x12]._reg * 8;
    const uint32_t mRDIFF = spu->reverbRegisters[0x13]._reg * 8;
    const uint32_t mLCOMB3 = spu->reverbRegisters[0x14]._reg * 8;
    const uint32_t mRCOMB3 = spu->reverbRegisters[0x15]._reg * 8;
    const uint32_t mLCOMB4 = spu->reverbRegisters[0x16]._reg * 8;
    const uint32_t mRCOMB4 = spu->reverbRegisters[0x17]._reg * 8;
    const uint32_t dLDIFF = spu->reverbRegisters[0x18]._reg * 8;
    const uint32_t dRDIFF = spu->reverbRegisters[0x19]._reg * 8;
    const uint32_t mLAPF1 = spu->reverbRegisters[0x1A]._reg * 8;
    const uint32_t mRAPF1 = spu->reverbRegisters[0x1B]._reg * 8;
    const uint32_t mLAPF2 = spu->reverbRegisters[0x1C]._reg * 8;
    const uint32_t mRAPF2 = spu->reverbRegisters[0x1D]._reg * 8;

    const float vLIN = intToFloat(spu->reverbRegisters[0x1E]._reg);
    const float vRIN = intToFloat(spu->reverbRegisters[0x1F]._reg);

    float Lin = vLIN * intToFloat(std::get<0>(input));
    float Rin = vRIN * intToFloat(std::get<1>(input));

#define R(addr) (read(spu, (addr)))
#define W(addr, data) (write(spu, (addr), (data)))

    W(mLSAME, (Lin + R(dLSAME) * vWALL - R(mLSAME - 2)) * vIIR + R(mLSAME - 2));
    W(mRSAME, (Rin + R(dRSAME) * vWALL - R(mRSAME - 2)) * vIIR + R(mRSAME - 2));

    W(mLDIFF, (Lin + R(dRDIFF) * vWALL - R(mLDIFF - 2)) * vIIR + R(mLDIFF - 2));
    W(mRDIFF, (Rin + R(dLDIFF) * vWALL - R(mRDIFF - 2)) * vIIR + R(mRDIFF - 2));

    float Lout = vCOMB1 * R(mLCOMB1) + vCOMB2 * R(mLCOMB2) + vCOMB3 * R(mLCOMB3) + vCOMB4 * R(mLCOMB4);
    float Rout = vCOMB1 * R(mRCOMB1) + vCOMB2 * R(mRCOMB2) + vCOMB3 * R(mRCOMB3) + vCOMB4 * R(mRCOMB4);

    Lout = Lout - (vAPF1 * R(mLAPF1 - dAPF1));
    W(mLAPF1, Lout);
    Lout = Lout * vAPF1 + R(mLAPF1 - dAPF1);
    Rout = Rout - (vAPF1 * R(mRAPF1 - dAPF1));
    W(mRAPF1, Rout);
    Rout = Rout * vAPF1 + R(mRAPF1 - dAPF1);

    Lout = Lout - (vAPF2 * R(mLAPF2 - dAPF2));
    W(mLAPF2, Lout);
    Lout = Lout * vAPF2 + R(mLAPF2 - dAPF2);
    Rout = Rout - (vAPF2 * R(mRAPF2 - dAPF2));
    W(mRAPF2, Rout);
    Rout = Rout * vAPF2 + R(mRAPF2 - dAPF2);

    spu->reverbCurrentAddress = wrap(spu, spu->reverbCurrentAddress + 2);

    Lout = clamp(Lout, -1.f, 1.f);
    Rout = clamp(Rout, -1.f, 1.f);

    return std::make_tuple(clamp((floatToInt(Lout) * spu->reverbVolume.getLeft()) >> 15, INT16_MIN, INT16_MAX),
                           clamp((floatToInt(Rout) * spu->reverbVolume.getRight()) >> 15, INT16_MIN, INT16_MAX));
}
}  // namespace spu