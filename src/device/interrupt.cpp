#include "interrupt.h"
#include "system.h"

using namespace interrupt;

Interrupt::Interrupt(System* sys) : sys(sys) {}
void Interrupt::trigger(IrqNumber irq) {
    if (irq > 10) return;
    status._reg |= (1 << irq);
    step();
}

bool Interrupt::interruptPending() { return (status._reg & mask._reg) ? true : false; }

std::string Interrupt::getMask() {
    std::string buf;
    buf.resize(11);
    buf[0] = mask.vblank ? 'V' : '-';
    buf[1] = mask.gpu ? 'G' : '-';
    buf[2] = mask.cdrom ? 'C' : '-';
    buf[3] = mask.dma ? 'D' : '-';
    buf[4] = mask.timer0 ? '0' : '-';
    buf[5] = mask.timer1 ? '1' : '-';
    buf[6] = mask.timer2 ? '2' : '-';
    buf[7] = mask.controller ? 'C' : '-';
    buf[8] = mask.sio ? 'I' : '-';
    buf[9] = mask.spu ? 'S' : '-';
    buf[10] = mask.lightpen ? 'L' : '-';
    buf[11] = 0;
    return buf;
}

std::string Interrupt::getStatus() {
    std::string buf;
    buf.resize(11);
    buf[0] = status.vblank ? 'V' : '-';
    buf[1] = status.gpu ? 'G' : '-';
    buf[2] = status.cdrom ? 'C' : '-';
    buf[3] = status.dma ? 'D' : '-';
    buf[4] = status.timer0 ? '0' : '-';
    buf[5] = status.timer1 ? '1' : '-';
    buf[6] = status.timer2 ? '2' : '-';
    buf[7] = status.controller ? 'C' : '-';
    buf[8] = status.sio ? 'I' : '-';
    buf[9] = status.spu ? 'S' : '-';
    buf[10] = status.lightpen ? 'L' : '-';
    buf[11] = 0;
    return buf;
}

void Interrupt::step() {
    // notify cop0
    sys->cpu->cop0.cause.interruptPending = interruptPending() ? 4 : 0;
}

uint8_t Interrupt::read(uint32_t address) {
    if (address < 0x2) return status._byte[address];
    if (address >= 0x4 && address < 0x6) return mask._byte[address - 4];
    return 0;
}

void Interrupt::write(uint32_t address, uint8_t data) {
    if (address < 0x2) {
        status._byte[address] &= data;  // write 0 to ACK
    }
    if (address >= 0x4 && address < 0x6) mask._byte[address - 4] = data;

    step();
}