#include "memory_card.h"
#include <fmt/core.h>
#include "config.h"

namespace peripherals {

MemoryCard::MemoryCard(int port) : AbstractDevice(Type::MemoryCard, port) { verbose = config.debug.log.memoryCard; }

uint8_t MemoryCard::handle(uint8_t byte) {
    if (state == 0) command = Command::None;

    if (!inserted) {
        return 0xff;
    }

    if (verbose >= 3) fmt::print("[MEMCARD_{}] state {}\n", port, state);
    if (command == Command::Read) return handleRead(byte);
    if (command == Command::Write) return handleWrite(byte);
    if (command == Command::ID) return handleId(byte);

    switch (state) {
        case 0:
            if (byte == 0x81) {
                state++;
                return 0xff;
            }
            return 0xff;

        case 1: {
            state++;
            if (byte == 'R') {
                command = Command::Read;
            } else if (byte == 'W') {
                command = Command::Write;
            } else if (byte == 'S') {
                command = Command::ID;
            } else {
                if (verbose >= 1) fmt::print("[MEMCARD_{}] Unsupported command 0x{:02x}\n", port, byte);
                state = 0;
            }
            uint8_t ret = flag._reg;
            flag.error = 0;
            return ret;
        }

        default: state = 0; return 0xff;
    }
}

uint8_t MemoryCard::handleRead(uint8_t byte) {
    if (state >= 10 && state < 10 + 128) {
        int n = state - 10;
        uint8_t d = data[(address._reg * 128) + n];
        checksum ^= d;
        state++;
        return d;
    }

    switch (state) {
        case 2: state++; return 0x5A;  // Card ID1
        case 3: state++; return 0x5D;  // Card ID2
        case 4:
            address.write(1, byte);  // MSB
            state++;
            return 0;
        case 5:
            address.write(0, byte);  // LSB
            // Check if address is out of bounds
            if (address._reg > 1024 - 1) {
                if (verbose >= 1) fmt::print("[MEMCARD_{}] Out of bounds read 0x{:04x}\n", port, address._reg);
                address._reg &= 0x3ff;
            } else {
                if (verbose >= 2) fmt::print("[MEMCARD_{}] Reading 0x{:04x}\n", port, address._reg);
            }
            state++;
            return 0;

        case 6: state++; return 0x5C;  // ACK 1
        case 7: state++; return 0x5D;  // ACK 2
        case 8:
            state++;
            checksum = address.read(1);
            return address.read(1);  // Address MSB
        case 9:
            state++;
            checksum ^= address.read(0);
            return address.read(0);  // Address LSB
        case 138: state++; return checksum;
        case 139:
            state = 0;
            command = Command::None;
            return 'G';

        default:
            state = 0;
            command = Command::None;
            return 0xff;
    }
}

uint8_t MemoryCard::handleWrite(uint8_t byte) {
    if (state >= 6 && state < 6 + 128) {
        int n = state - 6;
        data[(address._reg * 128) + n] = byte;
        checksum ^= byte;
        state++;
        return 0;
    }

    switch (state) {
        case 2: state++; return 0x5A;  // Card ID1
        case 3: state++; return 0x5D;  // Card ID2
        case 4:
            address.write(1, byte);  // MSB
            state++;
            return 0;
        case 5:
            address.write(0, byte);  // LSB
            checksum = address.read(1);
            checksum ^= address.read(0);

            writeStatus = WriteStatus::Good;

            // Check if address is out of bounds
            if (address._reg > 1024 - 1) {
                flag.error = 1;
                if (verbose >= 1) fmt::print("[MEMCARD_{}] Out of bounds write 0x{:04x}\n", port, address._reg);
                address._reg &= 0x3ff;

                writeStatus = WriteStatus::BadSector;
            } else {
                if (verbose >= 2) fmt::print("[MEMCARD_{}] Write 0x{:04x}\n", port, address._reg);
            }
            state++;
            return 0;

        case 134:
            state++;
            if (byte != checksum) {
                flag.error = 1;
                writeStatus = WriteStatus::BadChecksum;
            }
            return 0x00;

        case 135: state++; return 0x5C;
        case 136: state++; return 0x5D;
        case 137:
            dirty = true;
            flag.fresh = 0;
            state = 0;
            command = Command::None;

            bus.notify(Event::Controller::MemoryCardContentsChanged{port});

            return static_cast<uint8_t>(writeStatus);

        default:
            state = 0;
            command = Command::None;
            return 0xff;
    }
}

uint8_t MemoryCard::handleId(uint8_t byte) {
    (void)byte;
    if (verbose >= 1) fmt::print("[MEMCARD_{}] Unsupported ID command\n", port);
    command = Command::None;
    return 0xff;
}

}  // namespace peripherals