#include <iostream>

#include "Server.h"
#include "Constants.h"
#include "Utils.h"

void Server::processMessage(sf::IpAddress address, unsigned short port, sf::Packet &inPacket) {
	sf::Packet responsePacket;
	responsePacket << PROTOCOL_VERSION;

	sf::Uint8 command;
	inPacket >> command;

	if (handleUnverifiedCommands(command, address, port, inPacket, responsePacket))
		return;

	sf::Uint32 givenClientID = -1;
	inPacket >> givenClientID;

	if (handleVerifiedCommands(command, givenClientID, address, port, inPacket, responsePacket))
		return;

	Connection * con = findClientConnection(address, port);
	if (con == NULL)
		return;

	if (handleConnectedCommands(command, givenClientID, address, port, inPacket, responsePacket)) {
		con->updateLastPacketTime();
		return;
	}

	responsePacket << ResponseCode::ErroneousRequest;
	send(address, port, responsePacket);
}

// Commands that don't require any authentication
bool Server::handleUnverifiedCommands(sf::Uint8 command, sf::IpAddress address, unsigned short port, 
		sf::Packet &inPacket, sf::Packet &responsePacket) {

	switch (command) {
	case 0:
		// heartbeat response
		// keep-alive
		break;
	case RequestCode::RequestConnection:
		verifyConnection(address, port, responsePacket);
		break;

	case RequestCode::PTSNotification:
		std::cout << "got PTS notification!" << std::endl;
		// requests ping to location in packet
		if (address == punchthrough_address)	// only accept these packets from PTS
			pingClientOnPTSRequest(inPacket);
		break;

	default:
		return false;
	}
	return true;
}

// Commands that require an ID to accompany the request
bool Server::handleVerifiedCommands(sf::Uint8 command, sf::Uint32 clientID, sf::IpAddress address, unsigned short port,
		sf::Packet &inPacket, sf::Packet &responsePacket) {

	if (clientID != generateClientID(address, port)) {
		responsePacket << ResponseCode::ConnectionRequired;
		send(address, port, responsePacket);
		return true;
	}

	switch (command) {
	case RequestCode::VerifyConnection:
		openConnection(address, port, responsePacket, clientID);
		break;

	case RequestCode::RequestDisconnection:
		closeConnection(address, port, responsePacket, clientID, false);
		break;

	default:
		return false;
	}

	return true;
}

// Commands that require a connection to be set up
bool Server::handleConnectedCommands(sf::Uint8 command, sf::Uint32 clientID, sf::IpAddress address, unsigned short port,
		sf::Packet &inPacket, sf::Packet &responsePacket) {

	switch (command) {
	case RequestCode::BroadcastMessage:
		broadcastMessage(clientID, address, port, inPacket, responsePacket);
		break;

	case RequestCode::UpdatePlayer:
		// unused, should be hooked up to game logic
		break;

	default:
		return false;
	}

	return true;
}