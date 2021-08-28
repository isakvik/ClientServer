#pragma once
#include "Constants.h"

using namespace Network;

unsigned long now_millis();
void sleep_millis(int ms);
unsigned int fnv1a(const void* data, size_t numBytes, unsigned int hash = FNV1A_SEED);

bool receiveWithTimeout(sf::UdpSocket& socket, sf::Packet& packet, sf::IpAddress address, unsigned short port, int timeout_ms, const char* why);

// packet verification
/*
bool verifyPacket(sf::Packet& packet);
void prependPacketVerification(sf::Packet& packet);
*/