#pragma once
#include <queue>

#include <SFML/Main.hpp>
#include <SFML/Network.hpp>

#include "Constants.h"

using namespace Network;

class Connection {
public:
	enum class State { // unused
		NotConnected,
		Connecting,
		Connected
	};

	State currentState = State::NotConnected;

	// used in server only
	int clientID;

	sf::IpAddress address;
	unsigned short port;

	int lastReceivedPacketTime;

	////////////// methods

	Connection(sf::IpAddress address, unsigned short port);
	Connection(sf::IpAddress address, unsigned short port, int clientID);
	~Connection();

	void updateLastPacketTime();
	int timeSinceLastPacket();

	/*
	int open(unsigned short port);
	int receive(sf::Packet& in);
	int send(sf::Packet& packet);

private:
	sf::UdpSocket * socket;
	*/
};

