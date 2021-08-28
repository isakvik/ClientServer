#include <iostream>

#include "Connection.h"
#include "Constants.h"
#include "Utils.h"

using namespace Network;

Connection::Connection(sf::IpAddress address, unsigned short port) {
	this->address = address;
	this->port = port;

	lastReceivedPacketTime = now_millis(); // critical for timing out newly opened connections
}

Connection::Connection(sf::IpAddress address, unsigned short port, int clientID) {
	this->address = address;
	this->port = port;
	this->clientID = clientID;

	lastReceivedPacketTime = now_millis(); // critical for timing out newly opened connections
}

Connection::~Connection() {
}

void Connection::updateLastPacketTime() {
	lastReceivedPacketTime = now_millis();
}

int Connection::timeSinceLastPacket() {
	return now_millis() - lastReceivedPacketTime;
}

bool operator== (const Connection &n1, const Connection &n2) {
	return n1.address == n2.address
		&& n1.port == n2.port;
}