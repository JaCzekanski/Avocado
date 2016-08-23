#include <SDL_net.h>
#include "mips.h"

enum class PacketState {
	waitingForStart,
	reading,
	waitingForChecksum1,
	waitingForChecksum2
};

class GdbStub {
private:
	static const int GDB_PORT = 1234;
	static const int BUFFER_SIZE = 2048;

	IPaddress ip;
	TCPsocket listener = nullptr;
	TCPsocket client = nullptr;
	SDLNet_SocketSet socketSet = nullptr;

	// Communication
	PacketState state = PacketState::waitingForStart;
	char inBuffer[BUFFER_SIZE];
	int inBufferPos = 0;
	uint8_t calculatedChecksum;
	uint8_t checksum;

	char outBuffer[BUFFER_SIZE];
	int outBufferPos = 0;
	uint8_t outChecksum = 0;
	int getDebugChar();
	void onClientDisconnect();

	// Low level write functions
	char toHex( char i );
	char toInt( char i );
	void appendToBuffer(char c, bool calcChecksum = false);
	void write8( uint8_t r );
	void write32( uint32_t r );
	void writeString( const char *str );
	void flushBuffer();

	bool checkForIncommingConnections();
	bool getPacket();

	void beginPacket();
	void endPacket();

	// GDB Packets
	void dumpRegistersPacket(mips::CPU& cpu);
	void readMemoryPacket(mips::CPU& cpu);
	void continuePacket(mips::CPU& cpu);
	void stepPacket(mips::CPU& cpu);

public:
	bool sendBreak = false;
	bool initialize();
	void uninitialize();
	void handle(mips::CPU& cpu);
};







