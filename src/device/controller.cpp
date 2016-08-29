#include "controller.h"
#include "../mips.h"
#include <stdlib.h>

namespace device {
namespace controller {
void Controller::handleByte(uint8_t byte) {
    static bool selected = false;
	// Handle only controller 1
	if (control._reg & (1 << 13)) return;

    if (byte == 0x01 && !selected) {
        fifo.push_back(0xff);
        selected = true;
    }
    if (byte == 0x42 && selected) {
        fifo.push_back(0x41);
        fifo.push_back(0x5a);
        fifo.push_back(~state._byte[0]);
        fifo.push_back(~state._byte[1]);

        //printf("JOYPAD --> 0x%04x\n", state._reg);
        selected = false;
    }
}

Controller::Controller() {}

void Controller::step() {}

uint8_t Controller::read(uint32_t address) {
    if (address == 0 && !fifo.empty()) {  // RX

        uint8_t value = fifo.front();
        fifo.pop_front();

		((mips::CPU*)_cpu)->interrupt->IRQ(7);
        return value;
    }
    if (address == 4) {      
		// ((!fifo.empty()) << 1) |
        return (1 << 0) | ((rand()&1) << 1) | (0 << 2) | (1 << 7);
    }
	if (address >= 8 && address < 10) return mode._byte[address - 8];
    if (address >= 10 && address < 12) return control._byte[address - 10];
    if (address >= 14 && address < 16) return baud._byte[address - 14];
    return 0xff;
}

void Controller::write(uint32_t address, uint8_t data) {
    if (address == 0) handleByte(data);
	else if (address >= 8 && address < 10) mode._byte[address - 8] = data;
	else if (address >= 10 && address < 12) control._byte[address - 10] = data; // mask bits 4 & 6
	else if (address >= 14 && address < 16) baud._byte[address - 14] = data;
}
}
}
