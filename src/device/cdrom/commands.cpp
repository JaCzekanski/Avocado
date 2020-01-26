#include <fmt/core.h>
#include "cdrom.h"
#include "utils/bcd.h"

namespace device {
namespace cdrom {
void CDROM::cmdGetstat() {
    postInterrupt(3);
    writeResponse(stat._reg);

    if (verbose) fmt::print("CDROM: cmdGetstat -> 0x{:02x}\n", CDROM_response[0]);
}

void CDROM::cmdSetloc() {
    uint8_t minute = bcd::toBinary(readParam());
    uint8_t second = bcd::toBinary(readParam());
    uint8_t sector = bcd::toBinary(readParam());

    seekSector = sector + (second * 75) + (minute * 60 * 75);

    postInterrupt(3);
    writeResponse(stat._reg);

    if (verbose) fmt::print("CDROM: cmdSetloc(min = {}, sec = {}, sect = {})\n", minute, second, sector);
}

void CDROM::cmdPlay() {
    // Play NOT IMPLEMENTED
    disc::Position pos;
    if (!CDROM_params.empty()) {
        int track = readParam();  // param or setloc used
        if (track >= (int)disc->getTrackCount()) {
            fmt::print("CDROM: Invalid PLAY track parameter ({})\n", track);
            return;
        }
        pos = disc->getTrackStart(track);
        if (verbose) fmt::print("CDROM: PLAY (track: {})\n", track);
    } else {
        pos = disc::Position::fromLba(seekSector);
    }

    readSector = pos.toLba();

    stat.setMode(StatusCode::Mode::Playing);

    postInterrupt(3);
    writeResponse(stat._reg);

    if (verbose) fmt::print("CDROM: cmdPlay (pos: {})\n", pos.toString());
}

void CDROM::cmdReadN() {
    readSector = seekSector;

    stat.setMode(StatusCode::Mode::Reading);

    postInterrupt(3);
    writeResponse(stat._reg);

    if (verbose) fmt::print("CDROM: cmdReadN\n");
}

void CDROM::cmdMotorOn() {
    stat.motor = 1;

    postInterrupt(3);
    writeResponse(stat._reg);

    postInterrupt(2);
    writeResponse(stat._reg);

    if (verbose) fmt::print("CDROM: cmdMotorOn\n");
}

void CDROM::cmdStop() {
    stat.setMode(StatusCode::Mode::None);
    stat.motor = 0;

    postInterrupt(3);
    writeResponse(stat._reg);

    postInterrupt(2);
    writeResponse(stat._reg);

    if (verbose) fmt::print("CDROM: cmdStop\n");
}

void CDROM::cmdPause() {
    postInterrupt(3);
    writeResponse(stat._reg);

    stat.setMode(StatusCode::Mode::None);

    postInterrupt(2);
    writeResponse(stat._reg);

    if (verbose) fmt::print("CDROM: cmdPause\n");
}

void CDROM::cmdInit() {
    postInterrupt(3);
    writeResponse(stat._reg);

    stat.motor = 1;
    stat.setMode(StatusCode::Mode::None);

    mode._reg = 0;

    postInterrupt(2);
    writeResponse(stat._reg);

    if (verbose) fmt::print("CDROM: cmdInit\n");
}

void CDROM::cmdMute() {
    mute = true;
    postInterrupt(3);
    writeResponse(stat._reg);

    if (verbose) fmt::print("CDROM: cmdMute\n");
}

void CDROM::cmdDemute() {
    mute = false;
    postInterrupt(3);
    writeResponse(stat._reg);

    if (verbose) fmt::print("CDROM: cmdDemute\n");
}

void CDROM::cmdSetFilter() {
    filter.file = readParam();
    filter.channel = readParam();
    postInterrupt(3);
    writeResponse(stat._reg);

    if (verbose) fmt::print("CDROM: cmdSetFilter(file = 0x{:02x}, ch = 0x{:02x})\n", filter.file, filter.channel);
}

void CDROM::cmdSetmode() {
    uint8_t setmode = readParam();

    mode._reg = setmode;

    postInterrupt(3);
    writeResponse(stat._reg);

    if (verbose) fmt::print("CDROM: cmdSetmode(0x{:02x})\n", setmode);
}

void CDROM::cmdGetparam() {
    postInterrupt(3);
    writeResponse(stat._reg);
    writeResponse(mode._reg);
    writeResponse(0x00);
    writeResponse(filter.file);
    writeResponse(filter.channel);

    if (verbose) fmt::print("CDROM: cmdGetparam({})\n", dumpFifo(CDROM_response));
}

void CDROM::cmdSetSession() {
    uint8_t session = readParam();

    postInterrupt(3);
    writeResponse(stat._reg);

    postInterrupt(2);
    writeResponse(stat._reg);

    if (verbose) fmt::print("CDROM: cmdSetSession(0x{:02x})\n", session);
}

void CDROM::cmdSeekP() {
    readSector = seekSector;

    postInterrupt(3);
    writeResponse(stat._reg);

    stat.setMode(StatusCode::Mode::Seeking);

    postInterrupt(2);
    writeResponse(stat._reg);

    stat.setMode(StatusCode::Mode::None);

    if (verbose) fmt::print("CDROM: cmdSeekP\n");
}

void CDROM::cmdGetlocL() {
    if (trackType != disc::TrackType::DATA || rawSector.empty()) {
        postInterrupt(5);
        writeResponse(0x80);
        return;
    }

    postInterrupt(3);
    writeResponse(rawSector[12]);  // minute (track)
    writeResponse(rawSector[13]);  // second (track)
    writeResponse(rawSector[14]);  // sector (track)
    writeResponse(rawSector[15]);  // mode
    writeResponse(rawSector[16]);  // file
    writeResponse(rawSector[17]);  // channel
    writeResponse(rawSector[18]);  // sm
    writeResponse(rawSector[19]);  // ci

    if (verbose) {
        fmt::print(
            "CDROM: cmdGetlocL -> ("
            "pos = (0x{:02x}:0x{:02x}:0x{:02x}), "
            "mode = 0x{:x}, "
            "file = 0x{:x}, "
            "channel = 0x{:x}, "
            "submode = 0x{:x}, "
            "ci = 0x{:x}"
            ")\n",
            CDROM_response[0], CDROM_response[1], CDROM_response[2],  //
            CDROM_response[3], CDROM_response[4], CDROM_response[5],  //
            CDROM_response[6], CDROM_response[7]);
    }
}

void CDROM::cmdGetlocP() {
    postInterrupt(3);
    writeResponse(lastQ.data[0]);  // track
    writeResponse(lastQ.data[1]);  // index
    writeResponse(lastQ.data[2]);  // minute (track)
    writeResponse(lastQ.data[3]);  // second (track)
    writeResponse(lastQ.data[4]);  // sector (track)
    writeResponse(lastQ.data[6]);  // minute (disc)
    writeResponse(lastQ.data[7]);  // second (disc)
    writeResponse(lastQ.data[8]);  // sector (disc)

    if (verbose) {
        fmt::print("CDROM: cmdGetlocP -> ({})\n", dumpFifo(CDROM_response));
    }
}

void CDROM::cmdGetTN() {
    postInterrupt(3);
    writeResponse(stat._reg);
    writeResponse(bcd::toBcd(0x01));
    writeResponse(bcd::toBcd(disc->getTrackCount()));

    if (verbose) {
        fmt::print("CDROM: cmdGetTN -> ({})\n", dumpFifo(CDROM_response));
    }
}

void CDROM::cmdGetTD() {
    unsigned track = bcd::toBinary(readParam());

    if (track == 0) {  // end of last track
        auto diskSize = disc->getDiskSize();

        postInterrupt(3);
        writeResponse(stat._reg);
        writeResponse(bcd::toBcd(diskSize.mm));
        writeResponse(bcd::toBcd(diskSize.ss));
    } else if (track <= disc->getTrackCount()) {  // Start of n track
        auto start = disc->getTrackStart(track);

        postInterrupt(3);
        writeResponse(stat._reg);
        writeResponse(bcd::toBcd(start.mm));
        writeResponse(bcd::toBcd(start.ss));
    } else {  // error
        postInterrupt(5);
        writeResponse(0x10);

        if (verbose) {
            fmt::print("CDROM: GetTD(0x{:02x}): error\n", track);
        }
        return;
    }

    if (verbose) {
        fmt::print("CDROM: cmdGetTD(0x{:02x}) -> ({})\n", track, dumpFifo(CDROM_response));
    }
}

void CDROM::cmdSeekL() {
    readSector = seekSector;

    postInterrupt(3);
    writeResponse(stat._reg);

    stat.setMode(StatusCode::Mode::Seeking);

    postInterrupt(2);
    writeResponse(stat._reg);

    if (verbose) fmt::print("CDROM: cmdSeekL\n");
}

void CDROM::cmdTest() {
    uint8_t opcode = readParam();
    if (opcode == 0x03) {  // Force motor off, used in swap
        stat.motor = 0;
        postInterrupt(3);
        writeResponse(stat._reg);
    } else if (opcode == 0x04) {  // Read SCEx string
        stat.motor = 1;
        scexCounter = 0;
        postInterrupt(3);
        writeResponse(stat._reg);

        if (readSector < 1024) {
            scexCounter++;
        }
    } else if (opcode == 0x05) {  // Get SCEx counters
        postInterrupt(3);
        writeResponse(scexCounter);
        writeResponse(scexCounter);
    } else if (opcode == 0x20) {  // Get CDROM BIOS date/version (yy,mm,dd,ver)
        postInterrupt(3);
        writeResponse(0x94);
        writeResponse(0x09);
        writeResponse(0x19);
        writeResponse(0xc0);
    } else if (opcode == 0x22) {  // Get CDROM BIOS region
        // Simulate SCEA bios
        postInterrupt(3);
        const std::string region = "for U/C";
        for (auto c : region) {
            writeResponse(c);
        }
    } else {
        fmt::print("CDROM: Unimplemented test opcode (0x{:x})!\n", opcode);
        postInterrupt(5);
    }

    if (verbose) {
        fmt::print("CDROM: cmdTest(0x{:02x}) -> ({})\n", opcode, dumpFifo(CDROM_response));
    }
}

void CDROM::cmdGetId() {
    // Shell open
    if (stat.getShell()) {
        postInterrupt(5);
        writeResponse(0x11);
        writeResponse(0x80);
        return;
    }

    postInterrupt(3);
    writeResponse(stat._reg);

    // No CD
    if (disc->getTrackCount() == 0) {
        postInterrupt(5);
        writeResponse(stat._reg);
        writeResponse(0x40);
        for (int i = 0; i < 6; i++) writeResponse(0);
    }
    // Audio CD
    else if (disc->read(disc::Position(0, 2, 0)).second == disc::TrackType::AUDIO) {
        postInterrupt(5);
        writeResponse(0x0a);
        writeResponse(0x90);
        for (int i = 0; i < 6; i++) writeResponse(0);
    } else {
        // Game CD
        postInterrupt(2);
        writeResponse(0x02);
        writeResponse(0x00);
        writeResponse(0x20);
        writeResponse(0x00);
        writeResponse('S');
        writeResponse('C');
        writeResponse('E');
        writeResponse('A');  // 0x45 E, 0x41 A, 0x49 I
    }

    if (verbose) {
        fmt::print("CDROM: cmdGetId -> ({})\n", dumpFifo(CDROM_response));
    }
}

void CDROM::cmdReadS() {
    readSector = seekSector;

    audio.clear();
    stat.setMode(StatusCode::Mode::Reading);

    postInterrupt(3);
    writeResponse(stat._reg);

    if (verbose) fmt::print("CDROM: cmdReadS\n");
}

void CDROM::cmdReadTOC() {
    postInterrupt(3);
    writeResponse(stat._reg);

    postInterrupt(2);
    writeResponse(stat._reg);

    if (verbose) fmt::print("CDROM: cmdReadTOC\n");
}

void CDROM::cmdUnlock() {
    // Semi implemented
    postInterrupt(5);
    writeResponse(0x11);
    writeResponse(0x40);

    if (verbose) {
        fmt::print("CDROM: cmdUnlock -> ({})\n", dumpFifo(CDROM_response));
    }
}
}  // namespace cdrom
}  // namespace device
