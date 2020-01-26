#include "reverb.h"
#include <vector>
#include "adsr.h"
#include "sample.h"
#include "device/device.h"
#include "utils/math.h"

namespace spu {
uint32_t wrap(SPU* spu, uint32_t address) {
    const uint32_t reverbBase = spu->reverbBase._reg * 8;

    uint32_t rel = address - reverbBase;
    rel = rel % (spu->RAM_SIZE - reverbBase);

    return (reverbBase + rel) & 0x7fffe;
}

void write(SPU* spu, uint32_t address, Sample sample) {
    uint16_t clamped = sample;

    uint32_t addr = wrap(spu, spu->reverbCurrentAddress + address);
    spu->ram[addr + 0] = clamped & 0xff;
    spu->ram[addr + 1] = (clamped >> 8) & 0xff;
}

Sample read(SPU* spu, uint32_t address) {
    uint32_t addr = wrap(spu, spu->reverbCurrentAddress + address);
    uint16_t data = spu->ram[addr] | (spu->ram[addr + 1] << 8);
    return data;
}

std::tuple<int16_t, int16_t> doReverb(SPU* spu, std::tuple<int16_t, int16_t> input) {
    const auto REG = [spu](int r) {  //
        return spu->reverbRegisters[r]._reg;
    };
    const auto R = [spu](uint32_t addr) {  //
        return read(spu, addr);
    };
    const auto W = [spu](uint32_t addr, Sample sample) {  //
        return write(spu, addr, sample);
    };

    const uint32_t dAPF1 = REG(0x00) * 8;
    const uint32_t dAPF2 = REG(0x01) * 8;
    const Sample vIIR = REG(0x02);
    const Sample vCOMB1 = REG(0x03);
    const Sample vCOMB2 = REG(0x04);
    const Sample vCOMB3 = REG(0x05);
    const Sample vCOMB4 = REG(0x06);
    const Sample vWALL = REG(0x07);
    const Sample vAPF1 = REG(0x08);
    const Sample vAPF2 = REG(0x09);
    const uint32_t mLSAME = REG(0x0A) * 8;
    const uint32_t mRSAME = REG(0x0B) * 8;
    const uint32_t mLCOMB1 = REG(0x0C) * 8;
    const uint32_t mRCOMB1 = REG(0x0D) * 8;
    const uint32_t mLCOMB2 = REG(0x0E) * 8;
    const uint32_t mRCOMB2 = REG(0x0F) * 8;
    const uint32_t dLSAME = REG(0x10) * 8;
    const uint32_t dRSAME = REG(0x11) * 8;
    const uint32_t mLDIFF = REG(0x12) * 8;
    const uint32_t mRDIFF = REG(0x13) * 8;
    const uint32_t mLCOMB3 = REG(0x14) * 8;
    const uint32_t mRCOMB3 = REG(0x15) * 8;
    const uint32_t mLCOMB4 = REG(0x16) * 8;
    const uint32_t mRCOMB4 = REG(0x17) * 8;
    const uint32_t dLDIFF = REG(0x18) * 8;
    const uint32_t dRDIFF = REG(0x19) * 8;
    const uint32_t mLAPF1 = REG(0x1A) * 8;
    const uint32_t mRAPF1 = REG(0x1B) * 8;
    const uint32_t mLAPF2 = REG(0x1C) * 8;
    const uint32_t mRAPF2 = REG(0x1D) * 8;
    const Sample vLIN = REG(0x1E);
    const Sample vRIN = REG(0x1F);

    Sample Lin = vLIN * std::get<0>(input);
    Sample Rin = vRIN * std::get<1>(input);

    W(mLSAME, (Lin + R(dLSAME) * vWALL - R(mLSAME - 2)) * vIIR + R(mLSAME - 2));
    W(mRSAME, (Rin + R(dRSAME) * vWALL - R(mRSAME - 2)) * vIIR + R(mRSAME - 2));

    W(mLDIFF, (Lin + R(dRDIFF) * vWALL - R(mLDIFF - 2)) * vIIR + R(mLDIFF - 2));
    W(mRDIFF, (Rin + R(dLDIFF) * vWALL - R(mRDIFF - 2)) * vIIR + R(mRDIFF - 2));

    Sample Lout = vCOMB1 * R(mLCOMB1) + vCOMB2 * R(mLCOMB2) + vCOMB3 * R(mLCOMB3) + vCOMB4 * R(mLCOMB4);
    Sample Rout = vCOMB1 * R(mRCOMB1) + vCOMB2 * R(mRCOMB2) + vCOMB3 * R(mRCOMB3) + vCOMB4 * R(mRCOMB4);

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

    return std::make_tuple(                  //
        Lout * spu->reverbVolume.getLeft(),  //
        Rout * spu->reverbVolume.getRight()  //
    );
}
}  // namespace spu