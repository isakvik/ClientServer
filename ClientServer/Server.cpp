#include <iostream>
#include <random>			// std::random_device

#include <sstream>			// std::stringstream

#include "Server.h"
#include "Constants.h"
#include "Utils.h"


using namespace Network;

Server::Server(unsigned short port) {
	std::random_device rng;
	serverHashSeed = rng();

	open(port);
}

Server::~Server() {
}

void Server::open(unsigned short port) {
	serverSocket.setBlocking(false);
	if (serverSocket.bind(port, sf::IpAddress::Any) != sf::Socket::Done)
		throw std::exception("Could not bind to port! It might be in use already.");

	serverStartTime_ms = now_millis();
	std::cout << "server started at port " << port << " (seed 0x" << std::hex << serverHashSeed << std::dec << ")" << std::endl;
}

void Server::verifyConnection(sf::IpAddress address, unsigned short port, sf::Packet &responsePacket) {
	responsePacket << ResponseCode::ConnectionReceived;

	// TODO: don't always accept connections
	if (numConnectedClients > MAX_CLIENTS) {
		// TODO: stop connection attempts, give feedback...
		return;
	}

	sf::Uint32 clientID = generateClientID(address, port);
	responsePacket << clientID;

	// send used port back to client
	if (send(address, port, responsePacket)) {
		std::cout << "sent ID " << clientID << " to " << address << ":" << port << std::endl;
	}
}

void Server::openConnection(sf::IpAddress address, unsigned short port, sf::Packet &responsePacket, sf::Uint32 clientID) {

	// TODO: might not need to do same check twice
	if (numConnectedClients > MAX_CLIENTS) {
		return;
	}

	if (std::find(clientIDs.begin(), clientIDs.end(), clientID) != clientIDs.end()) {
		return;
	}

	connections.push_back(new Connection(address, port, clientID));
	clientIDs.push_back(clientID);
	numConnectedClients++;

	responsePacket << ResponseCode::DisplayText;
	responsePacket << "connected to server!";

	if (send(address, port, responsePacket)) {
		std::cout << "verified connection to " << address << ":" << port << " with ID " << clientID << std::endl;
	}
}

void Server::closeConnection(sf::IpAddress address, unsigned short port, sf::Packet &responsePacket, sf::Uint32 clientID, bool wasTimedOut) {
	int index = findClientIndex(address, port);
	if (index > -1) {
		closeConnection(responsePacket, index, wasTimedOut);
	}
	else {
		responsePacket << ResponseCode::ConnectionRequired;
		send(address, port, responsePacket);
	}
}

void Server::closeConnection(sf::Packet &responsePacket, int index, bool wasTimedOut) {

	sf::IpAddress address = connections[index]->address;
	unsigned short port = connections[index]->port;

	if (index > -1) {

		// free Connection object
		delete connections[index];
		connections.erase(connections.begin() + index);
		clientIDs.erase(clientIDs.begin() + index);

		numConnectedClients--;

		std::cout << address << ":" << port;
		
		// TODO: change disconnect reason to lookup table?
		responsePacket << ResponseCode::ForciblyDisconnected;
		if (wasTimedOut) {
			responsePacket << "Ping timeout";
			std::cout << " timed out." << std::endl;
		}
		else {
			responsePacket << "Disconnected by request";
			std::cout << " disconnected." << std::endl;
		}
	}

	send(address, port, responsePacket);
}

void Server::broadcastMessage(sf::Uint32 senderID, sf::IpAddress address, unsigned short port, 
		sf::Packet &inPacket, sf::Packet &responsePacket) {
	char message[512];
	inPacket >> message;
	std::cout << "<" << senderID << "> " << message << std::endl;
	for (unsigned int i = 0; i < connections.size(); i++) {
		if (connections[i]->clientID == senderID)
			continue;

		sf::Packet broadcastPacket;
		broadcastPacket << PROTOCOL_VERSION << ResponseCode::DisplayText << message;
		send(connections[i]->address, connections[i]->port, broadcastPacket);
	}
	responsePacket << true;
	send(address, port, responsePacket);
}

void Server::broadcastNewConnection(sf::Uint32 clientID) {
	for (unsigned int i = 0; i < connections.size(); i++) {
		if (connections[i]->clientID == clientID)
			continue;

		sf::Packet broadcastPacket;
		std::string message = clientID + " has joined.";
		broadcastPacket << PROTOCOL_VERSION << ResponseCode::DisplayText << message;
		send(connections[i]->address, connections[i]->port, broadcastPacket);
	}
}

void Server::updateConnections() {
	unsigned long now = now_millis();
	uptime_ms = now - serverStartTime_ms;

	/* if (connections.size() > 0) {
		//std::cout << "connections size: " << connections.size() << std::endl;
	} */

	bool sendPing = now - lastHeartbeat_ms > server_heartbeat_frequency_ms;

	for (unsigned int i = 0; i < connections.size(); i++) {
		sf::Packet outPacket;
		outPacket << PROTOCOL_VERSION;

		if (sendPing) {
			pingClient(connections[i]->address, connections[i]->port, outPacket);
			lastHeartbeat_ms = now;
		}
		else if (connections[i]->timeSinceLastPacket() > server_default_timeout_ms) { // ping timeout
			closeConnection(connections[i]->address, connections[i]->port, outPacket, connections[i]->clientID, true);
		}
	}
}

// This method blocks the current thread while waiting for a response from the punchthrough server.
bool Server::contactPunchthroughServer(int timeout_ms, int retries) {
	while (retries-- > 0) {
		// order: protocol, checksum, data
		sf::Packet responsePacket;
		responsePacket << PROTOCOL_VERSION << (unsigned char) 's' << (sf::Uint16) serverSocket.getLocalPort(); // this is a game server

		if (serverSocket.send(responsePacket, punchthrough_address, punchthrough_port) != sf::Socket::Done) {
			std::cout << "failed sending to PTS!" << std::endl;
			return false;
		}

		sf::Packet inPacket;
		if (!receiveWithTimeout(serverSocket, inPacket, punchthrough_address, punchthrough_port, timeout_ms, "server connecting to PTS")) {
			std::cout << "PTS timed out!" << std::endl;
			continue;
		}

		bool response;
		inPacket >> response;
		if (response) {
			std::cout << "game server registered at the PTS." << std::endl;
			return true;
		}
		std::cout << "packet was discarded from PTS!" << std::endl;
		return false;
	}

	return false;
}

void Server::updatePunchthroughServerState() {
	// TODO: not implemented, but so is the NAT punchthrough timeout!
}

// try to punch through NAT
// responsePacket: must contain client address and port to ping
void Server::pingClient(sf::IpAddress address, unsigned short port, sf::Packet &outPacket) {
	outPacket << ResponseCode::Ping;
	if (send(address, port, outPacket)) {
		std::cout << "pinged client at " << address << ":" << port << "." << std::endl;
	}
}

// try to punch through NAT
// responsePacket: must contain client address and port to ping
void Server::pingClientOnPTSRequest(sf::Packet &responsePacket) {

	sf::Uint32 clientAddress;
	sf::Uint16 clientPort;

	if (!(responsePacket >> clientAddress >> clientPort)) {
		std::cout << "could not extract address from PTS notification." << std::endl;
		return;
	}

	sf::IpAddress parsedAddress = sf::IpAddress(clientAddress);
	sf::Packet clientPingPacket;

	clientPingPacket << PROTOCOL_VERSION << ResponseCode::Ping;
	if (send(parsedAddress, clientPort, clientPingPacket)) {
		std::cout << "pinged client at " << parsedAddress << ":" << clientPort << "." << std::endl;
	}
}

bool Server::receive(sf::IpAddress &address, unsigned short &port, sf::Packet &packet) {
	if (serverSocket.receive(packet, address, port) != sf::Socket::Done)
		return false;
	//std::cout << "received " << packet.getDataSize() << " bytes from " << address << ":" << port << std::endl;
	return true;
}

bool Server::send(sf::IpAddress address, unsigned short port, sf::Packet &packet) {
	if (serverSocket.send(packet, address, port) != sf::Socket::Done){
		std::cout << "serverSocket send error - failed to send " << packet.getDataSize() << " bytes to " << address << ":" << port << std::endl;
		return false;
	}
	//std::cout << "sent " << packet.getDataSize() << " bytes to " << address << ":" << port << std::endl;
	return true;
}

// private methods

unsigned int Server::generateClientID(sf::IpAddress &address, unsigned short &port) {
	std::ostringstream oss;
	oss << address << port;

	const std::string tmp = oss.str();
	return fnv1a(tmp.c_str(), tmp.size(), serverHashSeed);
}

// std::find helper methods

int Server::findClientIndex(sf::Uint32 clientID) {
	std::vector<sf::Uint32>::iterator it;
	it = std::find(clientIDs.begin(), clientIDs.end(), clientID);

	if (it != clientIDs.end())
		return it - clientIDs.begin();
	return -1;
}

int Server::findClientIndex(sf::IpAddress address, unsigned short port) {
	std::vector<Connection*>::iterator it;
	it = std::find_if(connections.begin(), connections.end(), [address, port](Connection *con) {
		return con->address == address && con->port == port;
	});

	if (it != connections.end())
		return it - connections.begin();
	return -1;
}

Connection * Server::findClientConnection(sf::IpAddress address, unsigned short port) {
	std::vector<Connection*>::iterator it;
	it = std::find_if(connections.begin(), connections.end(), [address, port](Connection *con) {
		return con->address == address && con->port == port;
	});

	if (it != connections.end())
		return *it;
	return NULL;
}