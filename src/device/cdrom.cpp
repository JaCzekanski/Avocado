#include "cdrom.h"
#include "../mips.h"
#include <cstdio>

namespace device {
namespace cdrom {
CDROM::CDROM() {}

void CDROM::step() {}

uint8_t CDROM::read(uint32_t address) {
    if (address == 0) {
        uint8_t status = CDROM_index;
		status |= (0) << 6;
        status |= (!CDROM_response.empty()) << 5;
        status |= (!(CDROM_params.size() >= 16)) << 4;
        status |= (CDROM_params.empty()) << 3;
		status |= 0 << 2; // XA-ADPCM empty

		
		printf("CDROM%d.%d->R STATUS: 0x%02x\n", address, CDROM_index, status);
        return status;
    }
    if (address == 1) {
        uint8_t response = 0;
        if (!CDROM_response.empty()) {
            response = CDROM_response.front();
            CDROM_response.pop_front();
        }
		printf("CDROM%d.%d->R   RESP: 0x%02x\n", address, CDROM_index, response);
        return response;
    }
    if (address == 3) {  // type of response received
        uint8_t status = 0xe0;
        if (!CDROM_interrupt.empty()) {
            status |= CDROM_interrupt.front();
            CDROM_interrupt.pop_front();
        }

		printf("CDROM%d.%d->R    INT: 0x%02x\n", address, CDROM_index, status);
        return status;
    }
	printf("CDROM%d.%d->R    ?????\n", address, CDROM_index);
    return 0;
}

void CDROM::write(uint32_t address, uint8_t data) {
	if (address == 0) {
		CDROM_index = data & 3;
		return;
	}
    if (address == 1) {  // Command register
        CDROM_interrupt.clear();
        CDROM_response.clear();
        if (CDROM_index == 0) {
			if (data == 0x01)  // Getstat
			{
				static bool shellOpen = 1;
				CDROM_interrupt.push_back(3);
				CDROM_response.push_back(0b01000010 | (shellOpen << 4)); // playing CD-DA, no seeking, closed, ok, ok, motor on
				shellOpen = 0;
			}
			if (data == 0x02)  // Setloc
			{
				uint32_t a = CDROM_params.front(); CDROM_params.pop_front();
				uint32_t b = CDROM_params.front(); CDROM_params.pop_front();
				uint32_t c = CDROM_params.front(); CDROM_params.pop_front();
				printf("Setloc: %x %x %x\n", a, b, c);
				CDROM_interrupt.push_back(3);
				CDROM_response.push_back(0b01100010); // playing CD-DA, no seeking, closed, ok, ok, motor on
			}
			if (data == 0x06)  // ReadN
			{
				CDROM_interrupt.push_back(3);
				CDROM_response.push_back(0b01100010); // playing CD-DA, no seeking, closed, ok, ok, motor on

				CDROM_interrupt.push_back(1);
				CDROM_response.push_back(0b01100010); // playing CD-DA, no seeking, closed, ok, ok, motor on

				// 0x800 bytes

				for (int i = 0; i < 0x800; i++)
				{
					CDROM_response.push_back(i);
				}
			}
			if (data == 0x0e)  // Setmode
			{
				CDROM_interrupt.push_back(3);
				CDROM_response.push_back(0b01100010); // playing CD-DA, no seeking, closed, ok, ok, motor on
			}
			if (data == 0x09)  // Pause
			{
				CDROM_interrupt.push_back(3);
				CDROM_response.push_back(0b01100010); // playing CD-DA, no seeking, closed, ok, ok, motor on

				CDROM_interrupt.push_back(2);
				CDROM_response.push_back(0b01100010); // playing CD-DA, no seeking, closed, ok, ok, motor on
			}
			if (data == 0x15)  // SeekL
			{
				// Uses params from 0x02 setloc
				CDROM_interrupt.push_back(3);
				CDROM_response.push_back(0b01100010); // playing CD-DA, no seeking, closed, ok, ok, motor on

				CDROM_interrupt.push_back(2);
				CDROM_response.push_back(0b01100010); // playing CD-DA, no seeking, closed, ok, ok, motor on
			}
            if (data == 0x0a)  // Init
            {
                CDROM_interrupt.push_back(3);
                CDROM_response.push_back(0x20);  // stat
                CDROM_interrupt.push_back(2);
                CDROM_response.push_back(0x20);  // stat
            }
            if (data == 0x19)  // Test
            {
                if (CDROM_params.front() == 0x20)  // Get CDROM BIOS date/version (yy,mm,dd,ver)
                {
                    CDROM_params.pop_front();
                    CDROM_interrupt.push_back(3);
                    CDROM_response.push_back(0x97);
                    CDROM_response.push_back(0x01);
                    CDROM_response.push_back(0x10);
                    CDROM_response.push_back(0xc2);
                }
            }
			if (data == 0x1A)  //GetId
			{
				CDROM_interrupt.push_back(3);
				CDROM_response.push_back(0b00100010); // playing CD-DA, no seeking, closed, ok, ok, motor on


				CDROM_interrupt.push_back(2);
				CDROM_response.push_back(0x02);

				CDROM_interrupt.push_back(2);
				CDROM_response.push_back(0x00);

				CDROM_interrupt.push_back(2);
				CDROM_response.push_back(0x20);

				CDROM_interrupt.push_back(2);
				CDROM_response.push_back(0x00);


				CDROM_interrupt.push_back(2);
				CDROM_response.push_back(0x53);

				CDROM_interrupt.push_back(2);
				CDROM_response.push_back(0x43);

				CDROM_interrupt.push_back(2);
				CDROM_response.push_back(0x45);

				CDROM_interrupt.push_back(2);
				CDROM_response.push_back(0x45); //0x45 E, 0x41 A, 0x49 I
			}
        }
        CDROM_params.clear();

		printf("CDROM%d.%d<-W    CMD: 0x%02x\n", address, CDROM_index, data);
		return;
    }
    if (address == 2) {  // Parameter fifo
        if (CDROM_index == 0) {
            CDROM_params.push_back(data);
			printf("CDROM%d.%d<-W  PARAM: 0x%02x\n", address, CDROM_index, data);
			return;
        }
    }
	if (address == 3) { 
		if (CDROM_index == 1) { // Interrupt Flag Register R/W
			//CDROM_params.push_back(data);
		}
	}
	printf("CDROM%d.%d<-W         0x%02x\n", address, CDROM_index, data);
}
}
}