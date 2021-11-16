#pragma once
#include <array>
#include "abstract_device.h"

namespace peripherals {
struct MemoryCard : public AbstractDevice {
   private:
    enum class Command { Read, Write, ID, None };
    enum class WriteStatus : uint8_t { Good = 'G', BadChecksum = 'N', BadSector = 0xff };
    union Flag {
        struct {
            uint8_t : 2;
            uint8_t error : 1;
            uint8_t fresh : 1;
            uint8_t unknown : 1;
            uint8_t : 4;
        };
        uint8_t _reg;

        Flag() : error(0), fresh(1), unknown(1) {}
    };

    uint8_t handleRead(uint8_t byte);
    uint8_t handleWrite(uint8_t byte);
    uint8_t handleId(uint8_t byte);

    int verbose;
    Command command = Command::None;
    Flag flag;
    Reg16 address;  // Read/Write address (in 128B blocks)
    uint8_t checksum = 0;
    WriteStatus writeStatus = WriteStatus::Good;

   public:
    std::array<uint8_t, 128 * 1024> data;
    bool inserted = true;
    bool dirty = false;

    MemoryCard(int port);
    uint8_t handle(uint8_t byte) override;

    void setFresh() { flag.fresh = true; }
};
}  // namespace peripherals