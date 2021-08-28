#include <chrono>
#include <thread>
#include <iostream>

#include "Constants.h"

using namespace Network;

unsigned long now_millis() {
	return 
		std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()
		).count();
}

void sleep_millis(int ms) {
	return std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// hash a block of memory 
// http://create.stephan-brumme.com/fnv-hash/
unsigned int fnv1a(const void* data, size_t numBytes, unsigned int hash) {
	const byte* ptr = (const byte*) data;
	while (numBytes--)
		hash = (*ptr++ ^ hash) * FNV1A_PRIME;
	return hash;
}

/**
	This method checks a given socket every 33ms for a response, until the timeout interval has passed.
	Obviously, this blocks the thread.
*/
bool receiveWithTimeout(sf::UdpSocket& socket, sf::Packet& packet, sf::IpAddress address, unsigned short port, int timeout_ms, const char* why) {
	// backing up address data
	sf::IpAddress addressCopy = address;
	unsigned short portCopy = port;

	int start_t = now_millis();
	int duration_ms = 0;
	while (socket.receive(packet, address, port) != sf::Socket::Done) {
		duration_ms = now_millis() - start_t;

		if (duration_ms > timeout_ms) {
			// timed out
			std::cout << "call to " << addressCopy << ":" << portCopy << " timed out! call reason: " << why << std::endl;
			return false;
		}
		sleep_millis(33);
	}

	// packet now has content from receive command
	return true;
}

//////////// packet verification methods
/*
bool verifyPacket(sf::Packet& packet) {
	sf::Uint8 protocol;
	sf::Uint32 fnv1aChecksum;
	if (!(packet >> protocol >> fnv1aChecksum)) {
		return false;
	}

	if (protocol != PROTOCOL_VERSION) {
		return false;
	}

	int offset = sizeof(protocol) + sizeof(fnv1aChecksum);
	sf::Uint32 fnv1aHash = fnv1a((byte*) packet.getData() + offset, packet.getDataSize() - offset);
	if (fnv1aChecksum != fnv1aHash) {
		return false;
	}
	return true;
}

 **
	Because this method prepends a hash of the contents of the packet,
	this method has to be called after all the data has been inserted.
*
void prependPacketVerification(sf::Packet& packet) {
	sf::Uint32 checksum = fnv1a(packet.getData(), packet.getDataSize());
	int size = packet.getDataSize();

	byte* data = new byte[size];
	memcpy(data, packet.getData(), size);

	packet.clear();
	packet << PROTOCOL_VERSION << checksum;
	packet.append(data, size);

	delete data;
	data = NULL;
}
*/