#pragma once
#include <cassert>
#include <deque>
#include <memory>
#include "disc/disc.h"
#include "fifo.h"

struct System;

namespace device {
namespace cdrom {

class CDROM {
    union StatusCode {
        enum class Mode { None, Reading, Seeking, Playing };
        struct {
            uint8_t error : 1;
            uint8_t motor : 1;
            uint8_t seekError : 1;
            uint8_t idError : 1;
            uint8_t shellOpen : 1;
            uint8_t read : 1;
            uint8_t seek : 1;
            uint8_t play : 1;
        };
        uint8_t _reg;

        void setMode(Mode mode) {
            error = seekError = idError = false;
            read = seek = play = false;
            motor = true;
            if (mode == Mode::Reading) {
                read = true;
            } else if (mode == Mode::Seeking) {
                seek = true;
            } else if (mode == Mode::Playing) {
                play = true;
            }
        }

        void setShell(bool opened) {
            shellOpen = opened;

            setMode(Mode::None);
            if (opened) {
                motor = false;
            }
        }

        bool getShell() const { return shellOpen; }

        StatusCode() : _reg(0) { shellOpen = true; }
    };

    union CDROM_Status {
        struct {
            uint8_t index : 2;
            uint8_t xaFifoEmpty : 1;         // 1 - NOT empty, 0 - empty, INVERSE LOGIC
            uint8_t parameterFifoEmpty : 1;  // 1 - empty, 0 - not empty
            uint8_t parameterFifoFull : 1;   // 1 - NOT full, 0 - full, INVERSE LOGIC
            uint8_t responseFifoEmpty : 1;   // 1 - NOT empty, 0 - empty, INVERSE LOGIC
            uint8_t dataFifoEmpty : 1;       // 1 - NOT empty, 0 - empty, INVERSE LOGIC
            uint8_t transmissionBusy : 1;
        };
        uint8_t _reg;

        CDROM_Status() : _reg(0x18) {}
    };

    union Mode {
        struct {
            uint8_t cddaEnable : 1;     // 0 - off, 1 - on
            uint8_t cddaAutoPause : 1;  // Pause upon End of track (CDDA)
            uint8_t cddaReport : 1;     // Report CDDA report interrupts (like cmdGetlocP)
            uint8_t xaFilter : 1;       // Match XA sectors matching Setfilter
            uint8_t ignoreBit : 1;      // dunno, something sector size related
            uint8_t sectorSize : 1;     // 0 - 0x800, 1 - 0x924
            uint8_t xaEnabled : 1;      // 0 - off, 1 - on
            uint8_t speed : 1;          // 0 - normal (75 sectors / second), 1 - double speed
        };
        uint8_t _reg;

        Mode() : _reg(0) {}
    };

    struct Filter {
        uint8_t file;
        uint8_t channel;

        Filter() : file(0), channel(0) {}

        template <class Archive>
        void serialize(Archive& ar) {
            ar(file, channel);
        }
    };

    // Hack
    struct irq_response_t {
        uint8_t irq;
        fifo<uint8_t, 16> response;
        bool ack;
        int delay;

        template <class Archive>
        void serialize(Archive& ar) {
            ar(irq, response, ack, delay);
        }
    };

    using FIFO = fifo<uint8_t, 16>;

    int verbose = 1;

    CDROM_Status status;
    uint8_t interruptEnable = 0;
    FIFO CDROM_params;
    fifo<irq_response_t, 16> interruptQueue;

    Mode mode;
    Filter filter;
    uint8_t volumeLeftToLeft = 0x80;    // BIOS never sets this registers with default values
    uint8_t volumeLeftToRight = 0;      // but Audio CDs can be played
    uint8_t volumeRightToLeft = 0;      // cdrom/volume-regs showed that these registers aren't
    uint8_t volumeRightToRight = 0x80;  // zeroed on reset, but have 80 0 0 80 values

    System* sys;
    int readSector = 0;
    int seekSector = 0;

    StatusCode stat;

    int scexCounter = 0;

    void cmdGetstat();
    void cmdSetloc();
    void cmdPlay();
    void cmdReadN();
    void cmdMotorOn();
    void cmdStop();
    void cmdPause();
    void cmdInit();
    void cmdMute();
    void cmdDemute();
    void cmdSetFilter();
    void cmdSetmode();
    void cmdGetparam();
    void cmdGetlocL();
    void cmdGetlocP();
    void cmdGetTN();
    void cmdGetTD();
    void cmdSeekL();
    void cmdTest();
    void cmdGetId();
    void cmdReadS();
    void cmdReadTOC();
    void cmdUnlock();
    void cmdSetSession();
    void cmdSeekP();
    void handleCommand(uint8_t cmd);

    void writeResponse(uint8_t byte);

    uint8_t readParam() {
        uint8_t param = CDROM_params.get();
        return param;
    }

    int busyFor = 0;

    void postInterrupt(int irq, int delay = 50000) {
        interruptQueue.add(irq_response_t{
            .irq = (uint8_t)irq,  //
            .response = {},       //
            .delay = delay        //
        });
    }

    std::string dumpFifo(const FIFO& f);
    std::pair<int16_t, int16_t> mixSample(std::pair<int16_t, int16_t> sample);

    void handleSector();

   public:
    std::deque<std::pair<int16_t, int16_t>> audio;
    std::vector<uint8_t> rawSector;

    std::vector<uint8_t> dataBuffer;
    int dataBufferPointer;  // for DMA

    bool isBufferEmpty();
    uint8_t readByte();

    int readcnt = 0;
    disc::TrackType trackType;
    std::unique_ptr<disc::Disc> disc;
    disc::SubchannelQ lastQ;
    bool mute = false;
    int previousTrack;  // for CDDA autopause

    CDROM(System* sys);
    void step(int cycles);
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);

    void setShell(bool opened) {
        stat.setShell(opened);

        if (opened) {
            if (disc) {
                postInterrupt(0x05, 1000);
                writeResponse(0x16);
                writeResponse(0x08);
            }
        }
    }
    bool getShell() const { return stat.getShell(); }
    void ackMoreData() {
        postInterrupt(1, 0);
        writeResponse(stat._reg);
    }

    bool discPresent() { return stat.getShell() == false && disc; }

    template <class Archive>
    void serialize(Archive& ar) {
        ar(status._reg, interruptEnable);
        ar(CDROM_params);
        //        ar(CDROM_response);
        ar(interruptQueue);
        ar(mode._reg);
        ar(filter);
        ar(readSector);
        ar(seekSector);
        ar(scexCounter);
        ar(stat._reg);
        ar(volumeLeftToLeft, volumeLeftToRight, volumeRightToLeft, volumeRightToRight);
        ar(audio);
        ar(rawSector);
        ar(dataBuffer);
        ar(dataBufferPointer);
        ar(readcnt);
        ar(trackType);
        ar(lastQ);
        ar(mute);
    }
};
}  // namespace cdrom
}  // namespace device
