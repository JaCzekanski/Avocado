#include "gdbStub.h"
#include <cstdio>
#include <cstring>

int GdbStub::getDebugChar() {
    int status = SDLNet_CheckSockets(socketSet, 0);
    if (status == -1 || status == 0) return -1;

    if (SDLNet_SocketReady(client) == 0) return -1;

    char data;
    int read = SDLNet_TCP_Recv(client, &data, 1);  // TODO: read more than 1 byte per call
    if (read <= 0) {
        onClientDisconnect();
        return -1;
    }
    return data;
}

void GdbStub::onClientDisconnect() {
    fprintf(stderr, "Client disconnected\n");

    if (client) {
        SDLNet_TCP_DelSocket(socketSet, client);
        SDLNet_TCP_Close(client);
        client = NULL;
    }
    SDLNet_FreeSocketSet(socketSet);
    socketSet = NULL;

    if (listener) SDLNet_TCP_Close(listener);
    state = PacketState::waitingForStart;
}

char GdbStub::toHex(char i) {
    static const char hexchars[] = "0123456789abcdef";

    return hexchars[i & 0xf];
}

char GdbStub::toInt(char i) {
    if (i >= '0' && i <= '9') return i - '0';
    if (i >= 'a' && i <= 'f') return 10 + (i - 'a');
    if (i >= 'A' && i <= 'F') return 10 + (i - 'A');

    // TODO: Assert
    return 0;
}

void GdbStub::appendToBuffer(char c, bool calcChecksum) {
    outBuffer[outBufferPos] = c;
    if (calcChecksum) outChecksum += c;
    if (outBufferPos < BUFFER_SIZE)
        outBufferPos++;
    else
        printf("Output buffer overflow!\n");
}

void GdbStub::write8(uint8_t r) {
    appendToBuffer(toHex(r >> 4), true);
    appendToBuffer(toHex(r), true);
}

void GdbStub::write32(uint32_t r) {
    for (int i = 0; i < 4; i++) {
        write8(r & 0xff);
        r >>= 8;
    }
}

void GdbStub::writeString(const char* str) {
    while (*str) appendToBuffer(*str++, true);
}

void GdbStub::flushBuffer() {
    outBuffer[outBufferPos] = 0;
    SDLNet_TCP_Send(client, outBuffer, outBufferPos);
    outBufferPos = 0;
}

bool GdbStub::checkForIncommingConnections() {
    if (listener == NULL) listener = SDLNet_TCP_Open(&ip);  // Remember to kill on exit!

    client = SDLNet_TCP_Accept(listener);
    if (client == NULL) return false;

    SDLNet_TCP_Close(listener);
    listener = NULL;

    auto clientInfo = SDLNet_TCP_GetPeerAddress(client);
    fprintf(stderr, "Connection from %s\n", SDLNet_ResolveIP(clientInfo));

    socketSet = SDLNet_AllocSocketSet(1);  // remember to delete previous instances!
    if (!socketSet) return false;

    SDLNet_TCP_AddSocket(socketSet, client);
    return true;
}

bool GdbStub::getPacket() {
    char c;
    while ((c = getDebugChar()) != -1) {
        switch (state) {
            case PacketState::waitingForStart:
                if (c == '$') {
                    state = PacketState::reading;
                    inBufferPos = 0;
                    checksum = 0;
                    calculatedChecksum = 0;
                }
                if (c == 0x03) {
                    sendBreak = true;
                    return false;
                }
                break;

            case PacketState::reading:
                if (c == '#') {
                    inBuffer[inBufferPos] = 0;
                    state = PacketState::waitingForChecksum1;
                    break;
                }
                inBuffer[inBufferPos] = c;
                if (inBufferPos - 2 < BUFFER_SIZE)
                    inBufferPos++;
                else {
                    printf("Input buffer overflow!");
                    appendToBuffer('-');
                    return false;
                }

                calculatedChecksum += c;
                break;

            case PacketState::waitingForChecksum1:
                checksum = toInt(c) << 4;
                state = PacketState::waitingForChecksum2;
                break;

            case PacketState::waitingForChecksum2:
                checksum |= toInt(c);
                state = PacketState::waitingForStart;

                if (calculatedChecksum != checksum) {
                    appendToBuffer('-');
                    flushBuffer();
                    return false;
                }
                appendToBuffer('+');
                return true;
        }
    }
    return false;
}

void GdbStub::beginPacket() {
    outChecksum = 0;
    appendToBuffer('$');
}

void GdbStub::endPacket() {
    appendToBuffer('#');
    appendToBuffer(toHex(outChecksum >> 4));
    appendToBuffer(toHex(outChecksum));
    flushBuffer();
}

void GdbStub::dumpRegistersPacket(mips::CPU& cpu) {
    // general regs
    for (size_t i = 0; i < 32; i++) {
        write32(cpu.reg[i]);
    }

    // control regs
    write32(cpu.cop0.status._reg);
    write32(cpu.lo);
    write32(cpu.hi);
    write32(cpu.cop0.badVaddr);
    write32(cpu.cop0.cause._reg);
    write32(cpu.PC);

    // control regs
    for (size_t i = 0; i < 32; i++) {
        writeString("xxxxxxxx");
    }
}

void GdbStub::readMemoryPacket(mips::CPU& cpu) {
    uint32_t address = 0;
    uint32_t length = 0;
    size_t i;

    for (i = 1; i < strlen(inBuffer); i++) {
        if (inBuffer[i] == ',') break;
        address <<= 4;
        address |= toInt(inBuffer[i]);
    }
    for (++i; i < strlen(inBuffer); i++) {
        length <<= 4;
        length |= toInt(inBuffer[i]);
    }

    for (i = 0; i < length; i++) {
        write8(cpu.readMemory8(address + i));
    }
}

void GdbStub::continuePacket(mips::CPU& cpu) {
    long address = 0;

    for (size_t i = 1; i < strlen(inBuffer); i++) {
        address <<= 4;
        address |= toInt(inBuffer[i]);
    }

    // writeString("S00");
    cpu.state = mips::CPU::State::run;
}

void GdbStub::stepPacket(mips::CPU& cpu) {
    long address = 0;

    for (size_t i = 1; i < strlen(inBuffer); i++) {
        address <<= 4;
        address |= toInt(inBuffer[i]);
    }

    // writeString("S00");
    cpu.state = mips::CPU::State::run;
}

bool GdbStub::initialize() {
    if (SDLNet_Init() < 0) return false;
    if (SDLNet_ResolveHost(&ip, NULL, GDB_PORT) == -1) return false;

    return true;
}

void GdbStub::uninitialize() {
    if (client) {
        SDLNet_TCP_DelSocket(socketSet, client);
        SDLNet_FreeSocketSet(socketSet);
        SDLNet_TCP_Close(client);
    }

    if (listener) {
        SDLNet_TCP_Close(listener);
    }

    SDLNet_Quit();
}

void GdbStub::handle(mips::CPU& cpu) {
    if (!client) {
        if (!checkForIncommingConnections()) return;
        // Stop CPU
        cpu.state = mips::CPU::State::pause;
    }

    if (sendBreak) {
        sendBreak = false;
        cpu.state = mips::CPU::State::pause;
        beginPacket();
        writeString("S00");
        endPacket();
    }

    if (!getPacket()) return;
    fprintf(stderr, "in  | %s\n", inBuffer);

    beginPacket();
    switch (inBuffer[0]) {
        case '?':  // send status
            writeString("S00");
            break;

        case 'g':  // dump registers
            dumpRegistersPacket(cpu);
            break;

        case 'G':               // write general registers - G XX..
            writeString("OK");  // TODO write registers
            break;

        case 'm':  // read memory - r addr,length
            readMemoryPacket(cpu);
            break;

        case 'M':               // write memory - M addr,length:XX
            writeString("OK");  // TODO write memory
            break;

        case 'c':  // continue - c [address]
            continuePacket(cpu);
            return;
            break;

        case 's':  // step - s [address]
            stepPacket(cpu);
            break;

        default:
            // fprintf(stderr, "-- %s\n", inBuffer);
            endPacket();
            return;
    }
    endPacket();
}