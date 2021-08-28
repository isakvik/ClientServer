#pragma once
#include <SFML/Network.hpp>

#include "Connection.h"
#include "Constants.h"

using namespace Network;

const int client_default_timeout = 3000;
const int client_pts_timeout = client_default_timeout;
const int client_connection_retries = 5;

class Client {
public:

	bool connected = false;
	Connection * connection;

	sf::IpAddress serverAddress;
	unsigned short serverPort;

	Client();
	//Client(sf::IpAddress serverAddress, unsigned short serverPort);
	~Client();

	bool requestServerConnection(bool isPublic, int timeout_ms = client_default_timeout, int retries = client_connection_retries);
	bool verifyServerConnection(sf::IpAddress &address, unsigned short &port, sf::Packet &responsePacket);
	bool receive(sf::IpAddress &address, unsigned short &port, sf::Packet &packet);
	bool send(sf::IpAddress address, unsigned short port, sf::Packet &packet);

	void processResponse(sf::IpAddress address, unsigned short port, sf::Packet &inPacket);

	bool contactPunchthroughServer(sf::IpAddress address, unsigned short port, int timeout_ms = client_pts_timeout, int retries = client_connection_retries);
	// sends a 2 byte packet to the server.
	void pongServer(bool isResponse);

	sf::Uint32 getClientID();

private:
	sf::Uint32 clientID = 0;
	sf::UdpSocket connectionSocket;
};