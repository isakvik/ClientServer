#pragma once
#include <SFML/Network.hpp>

#include "Connection.h"
#include "Constants.h"

const int MAX_CLIENTS = 64;
const int server_updates_per_second = 30;
const int server_default_timeout_ms = 30000;
const int server_heartbeat_frequency_ms = 5000;
const int server_pts_timeout_ms = 3000;
const int server_pts_connection_retries = 5;

using namespace Network;

class Server {
public:
	int numConnectedClients;
	long uptime_ms = 0;	// updated in updateConnections()

	Server(unsigned short port);
	~Server();

	void open(unsigned short port);

	// ServerCommands.cpp
	void processMessage(sf::IpAddress address, unsigned short port, sf::Packet & inPacket);
	bool handleUnverifiedCommands(sf::Uint8 command, sf::IpAddress address, unsigned short port, 
		sf::Packet & inPacket, sf::Packet & responsePacket);
	bool handleVerifiedCommands(sf::Uint8 command, sf::Uint32 givenClientID, sf::IpAddress address, unsigned short port, 
		sf::Packet & inPacket, sf::Packet & responsePacket);
	bool handleConnectedCommands(sf::Uint8 command, sf::Uint32 givenClientID, sf::IpAddress address, unsigned short port,
		sf::Packet & inPacket, sf::Packet & responsePacket);

	
	void verifyConnection(sf::IpAddress sender, unsigned short senderPort, sf::Packet & responsePacket);
	void openConnection(sf::IpAddress sender, unsigned short senderPort, sf::Packet & responsePacket, sf::Uint32 clientID);
	void closeConnection(sf::IpAddress address, unsigned short port, sf::Packet & responsePacket, sf::Uint32 clientID, bool wasTimedOut);
	void closeConnection(sf::Packet & responsePacket, int index, bool wasTimedOut);

	void broadcastMessage(sf::Uint32 senderID, sf::IpAddress address, unsigned short port, sf::Packet & responsePacket, sf::Packet & inPacket);
	void broadcastNewConnection(sf::Uint32 clientID);

	bool send(sf::IpAddress address, unsigned short port, sf::Packet & packet);
	bool receive(sf::IpAddress & address, unsigned short & port, sf::Packet & packet);

	void updateConnections();

	bool contactPunchthroughServer(int timeout_ms = server_pts_timeout_ms, int retries = server_pts_connection_retries);
	void updatePunchthroughServerState();	// not implemented

	void pingClient(sf::IpAddress address, unsigned short port, sf::Packet &outPacket);
	void pingClientOnPTSRequest(sf::Packet &inPacket);

private:
	int serverHashSeed;	// used to generate client IDs
	sf::UdpSocket serverSocket;
	std::vector<Connection*> connections;
	std::vector<sf::Uint32> clientIDs;

	unsigned long serverStartTime_ms;
	unsigned long lastHeartbeat_ms;

	unsigned int generateClientID(sf::IpAddress &address, unsigned short &port);

	int findClientIndex(sf::Uint32 clientID);
	int findClientIndex(sf::IpAddress address, unsigned short port);
	Connection * findClientConnection(sf::IpAddress address, unsigned short port);
};