#include <cstdio>
#include "cdrom.h"
#include "utils/bcd.h"

namespace device {
namespace cdrom {
void CDROM::cmdGetstat() {
    postInterrupt(3);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdGetstat -> 0x%02x\n", CDROM_response[0]);
}

void CDROM::cmdSetloc() {
    uint8_t minute = bcd::toBinary(readParam());
    uint8_t second = bcd::toBinary(readParam());
    uint8_t sector = bcd::toBinary(readParam());

    seekSector = sector + (second * 75) + (minute * 60 * 75);

    postInterrupt(3);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdSetloc(min = %d, sec = %d, sect = %d)\n", minute, second, sector);
}

void CDROM::cmdPlay() {
    // Play NOT IMPLEMENTED
    disc::Position pos;
    if (!CDROM_params.empty()) {
        int track = readParam();  // param or setloc used
        if (track >= (int)disc->getTrackCount()) {
            printf("CDROM: Invalid PLAY track parameter (%d)\n", track);
            return;
        }
        pos = disc->getTrackStart(track);
        if (verbose) printf("CDROM: PLAY (track: %d)\n", track);
    } else {
        pos = disc::Position::fromLba(seekSector);
    }

    readSector = pos.toLba();

    stat.setMode(StatusCode::Mode::Playing);

    postInterrupt(3);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdPlay (pos: %s)\n", pos.toString().c_str());
}

void CDROM::cmdReadN() {
    readSector = seekSector;

    stat.setMode(StatusCode::Mode::Reading);

    postInterrupt(3);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdReadN\n");
}

void CDROM::cmdMotorOn() {
    stat.motor = 1;

    postInterrupt(3);
    writeResponse(stat._reg);

    postInterrupt(2);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdMotorOn\n");
}

void CDROM::cmdStop() {
    stat.setMode(StatusCode::Mode::None);
    stat.motor = 0;

    postInterrupt(3);
    writeResponse(stat._reg);

    postInterrupt(2);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdStop\n");
}

void CDROM::cmdPause() {
    postInterrupt(3);
    writeResponse(stat._reg);

    stat.setMode(StatusCode::Mode::None);

    postInterrupt(2);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdPause\n");
}

void CDROM::cmdInit() {
    postInterrupt(3);
    writeResponse(stat._reg);

    stat.motor = 1;
    stat.setMode(StatusCode::Mode::None);

    mode._reg = 0;

    postInterrupt(2);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdInit\n");
}

void CDROM::cmdMute() {
    mute = true;
    postInterrupt(3);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdMute\n");
}

void CDROM::cmdDemute() {
    mute = false;
    postInterrupt(3);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdDemute\n");
}

void CDROM::cmdSetFilter() {
    filter.file = readParam();
    filter.channel = readParam();
    postInterrupt(3);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdSetFilter(file = 0x%02x, ch = 0x%02x)\n", filter.file, filter.channel);
}

void CDROM::cmdSetmode() {
    uint8_t setmode = readParam();

    mode._reg = setmode;

    postInterrupt(3);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdSetmode(0x%02x)\n", setmode);
}

void CDROM::cmdSetSession() {
    uint8_t session = readParam();

    postInterrupt(3);
    writeResponse(stat._reg);

    postInterrupt(2);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdSetSession(0x%02x)\n", session);
}

void CDROM::cmdSeekP() {
    readSector = seekSector;

    postInterrupt(3);
    writeResponse(stat._reg);

    stat.setMode(StatusCode::Mode::Seeking);

    postInterrupt(2);
    writeResponse(stat._reg);

    stat.setMode(StatusCode::Mode::None);

    if (verbose) printf("CDROM: cmdSeekP\n");
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
        printf("CDROM: cmdGetlocL -> (");
        printf("pos = (%02x:%02x:%02x), ", CDROM_response[0], CDROM_response[1], CDROM_response[2]);
        printf("mode = 0x%x, ", CDROM_response[3]);
        printf("file = 0x%x, ", CDROM_response[4]);
        printf("channel = 0x%x, ", CDROM_response[5]);
        printf("submode = 0x%x, ", CDROM_response[6]);
        printf("ci = 0x%x", CDROM_response[7]);
        printf(")\n");
    }
}

void CDROM::cmdGetlocP() {
    auto pos = disc::Position::fromLba(readSector);

    int track = disc->getTrackByPosition(pos);
    auto posInTrack = pos - disc->getTrackStart(track);

    postInterrupt(3);
    writeResponse(bcd::toBcd(track));          // track
    writeResponse(0x01);                       // index
    writeResponse(bcd::toBcd(posInTrack.mm));  // minute (track)
    writeResponse(bcd::toBcd(posInTrack.ss));  // second (track)
    writeResponse(bcd::toBcd(posInTrack.ff));  // sector (track)
    writeResponse(bcd::toBcd(pos.mm));         // minute (disc)
    writeResponse(bcd::toBcd(pos.ss));         // second (disc)
    writeResponse(bcd::toBcd(pos.ff));         // sector (disc)

    if (verbose) {
        printf("CDROM: cmdGetlocP -> (%s)\n", dumpFifo(CDROM_response).c_str());
    }
}

void CDROM::cmdGetTN() {
    postInterrupt(3);
    writeResponse(stat._reg);
    writeResponse(bcd::toBcd(0x01));
    writeResponse(bcd::toBcd(disc->getTrackCount()));

    if (verbose) {
        printf("CDROM: cmdGetTN -> (%s)\n", dumpFifo(CDROM_response).c_str());
    }
}

void CDROM::cmdGetTD() {
    int track = readParam();
    postInterrupt(3);
    writeResponse(stat._reg);
    if (track == 0)  // end of last track
    {
        auto diskSize = disc->getDiskSize();
        if (verbose) printf("GetTD(0): minute: %d, second: %d\n", diskSize.mm, diskSize.ss);

        writeResponse(bcd::toBcd(diskSize.mm));
        writeResponse(bcd::toBcd(diskSize.ss));
    } else {  // Start of n track
        if (track > (int)disc->getTrackCount()) {
            // Error
            return;
        }

        auto start = disc->getTrackStart(track);

        if (verbose) printf("GetTD(%d): minute: %d, second: %d\n", track, start.mm, start.ss);
        writeResponse(bcd::toBcd(start.mm));
        writeResponse(bcd::toBcd(start.ss));
    }

    if (verbose) {
        printf("CDROM: cmdGetTD(0x%02x) -> ", track);
        dumpFifo(CDROM_response);
        printf("\n");
    }
}

void CDROM::cmdSeekL() {
    readSector = seekSector;

    postInterrupt(3);
    writeResponse(stat._reg);

    stat.setMode(StatusCode::Mode::Seeking);

    postInterrupt(2);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdSeekL\n");
}

void CDROM::cmdTest() {
    uint8_t opcode = readParam();
    if (opcode == 0x20)  // Get CDROM BIOS date/version (yy,mm,dd,ver)
    {
        postInterrupt(3);
        writeResponse(0x94);
        writeResponse(0x09);
        writeResponse(0x19);
        writeResponse(0xc0);
    } else if (opcode == 0x03) {  // Force motor off, used in swap
        stat.motor = 0;
        postInterrupt(3);
        writeResponse(stat._reg);
    } else {
        printf("Unimplemented test CDROM opcode (0x%x)!\n", opcode);
        postInterrupt(5);
    }

    if (verbose) {
        printf("CDROM: cmdTest(0x%02x) -> (%s)\n", opcode, dumpFifo(CDROM_response).c_str());
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
        writeResponse(0x08);
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
        printf("CDROM: cmdGetId -> (%s)\n", dumpFifo(CDROM_response).c_str());
    }
}

void CDROM::cmdReadS() {
    readSector = seekSector;

    audio.first.clear();
    audio.second.clear();
    stat.setMode(StatusCode::Mode::Reading);

    postInterrupt(3);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdReadS\n");
}

void CDROM::cmdReadTOC() {
    postInterrupt(3);
    writeResponse(stat._reg);

    postInterrupt(2);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdReadTOC\n");
}

void CDROM::cmdUnlock() {
    // Semi implemented
    postInterrupt(5);
    writeResponse(0x11);
    writeResponse(0x40);

    if (verbose) {
        printf("CDROM: cmdUnlock -> (%s)\n", dumpFifo(CDROM_response).c_str());
    }
}
}  // namespace cdrom
}  // namespace device
