#include "controller.h"
#include "../mips.h"

namespace device {
namespace controller {
	void Controller::handleByte(uint8_t byte)
	{
		static bool selected = false;
		if (byte == 0x01 && !selected) {
			fifo.push_back(0xff);
			selected = true;
		}
		if (byte == 0x42 && selected)
		{
			fifo.push_back(0x41);
			fifo.push_back(0x5a);
			fifo.push_back(~state._byte[0]);
			fifo.push_back(~state._byte[1]);
			printf("JOYPAD --> 0x%04x\n", state._reg);
			selected = false;
		}
		printf("JOYPAD: 0x%02x\n", byte);
	}

	Controller::Controller()
	{
	}

	void Controller::step()
	{
	}

	uint8_t Controller::read(uint32_t address)
	{
		if (address < 4 && !fifo.empty()) { // RX
			uint8_t value =  fifo.front();
			fifo.pop_front();
			return value;
		}
		if (address >= 4 && address < 8) { // STAT
//			if (address == 4) return 
//				(1 << 0) |
//				((!fifo.empty()) << 1) |
//				(1 << 2);
			return rand();
		}
		if (address >= 8 && address < 10) { // MODE
//			__debugbreak();
		}
		if (address >= 10 && address < 12) { // CTRL
//			__debugbreak();
			return rand();
		}
		if (address >= 14 && address < 16) { // BAUD
//			__debugbreak();
		}
		return 0;
	}

	void Controller::write(uint32_t address, uint8_t data)
	{
		if (address < 4) { // TX
			if (address == 0) {
				handleByte(data);
			}
			return;
		}

		if (address >= 8 && address < 12) { // MODE
			return;
		}
		if (address >= 10 && address < 12) { // CTRL
			control &= 0xff << (address - 12);
			control |= data << (address - 12);
			return;
		}
		if (address >= 14 && address < 16) { // BAUD
			return;
		}
	}


}
}
