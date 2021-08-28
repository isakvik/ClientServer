#include <thread>
#include <chrono>
#include <iostream>

#include "Client.h"
#include "Utils.h"

Client::Client() {
	connectionSocket.setBlocking(false);
	connectionSocket.bind(sf::Socket::AnyPort);
}

/*Client::Client(sf::IpAddress serverAddress, unsigned short serverPort) {
	connectionSocket.bind(sf::Socket::AnyPort);
}*/

Client::~Client() {
}

bool Client::requestServerConnection(bool isPublic, int timeout_ms, int retries) {
	while (retries-- > 0 && !connected) {
		// used in NAT punchthrough process, ensure first packet isn't lost
		if(isPublic)
			pongServer(false);

		sf::Packet requestPacket;
		requestPacket << PROTOCOL_VERSION << RequestCode::RequestConnection;
		if (!send(serverAddress, serverPort, requestPacket)) {
			std::cout << "failed to send packet!" << std::endl;
			return false;
		}

		sf::Packet inPacket;
		if (receiveWithTimeout(connectionSocket, inPacket, serverAddress, serverPort, 3000, "client requests server connection")) {
			processResponse(serverAddress, serverPort, inPacket);
			return true;
		}

		if (retries > 0)
			sleep_millis(100);
	}
	return false;
}

bool Client::verifyServerConnection(sf::IpAddress &address, unsigned short &port, sf::Packet &responsePacket) {
	sf::Uint32 clientID;
	if (!(responsePacket >> clientID)) {
		return false;
	}
	this->clientID = clientID;

	sf::Packet requestPacket;
	requestPacket << PROTOCOL_VERSION << RequestCode::VerifyConnection << clientID;
	if (!send(address, port, requestPacket)) {
		return false;
	}

	connection = new Connection(address, port);
	connection->currentState = Connection::State::Connected;
	connected = true;

	return true;
}

void Client::processResponse(sf::IpAddress address, unsigned short port, sf::Packet &inPacket) {
	sf::Uint8 protocol;
	if (inPacket >> protocol) {
		if (protocol == PROTOCOL_VERSION) {
			// TODO: move to a better place (processPacket)
			sf::Uint8 requestCode;
			inPacket >> requestCode;

			switch (requestCode) {
			case ResponseCode::Ping:
				pongServer(true);
				break;
			case ResponseCode::ConnectionReceived:
				verifyServerConnection(address, port, inPacket);
				break;
			case ResponseCode::ForciblyDisconnected:
				// TODO buffer overflow protection
				char reason[512];
				inPacket >> reason;
				std::cout << "Disconnected (reason: " << reason << ")" << std::endl;

				connection->currentState = Connection::State::NotConnected;
				break;

			case ResponseCode::ErroneousRequest:
				std::cout << "Function requested was not implemented." << std::endl;
				break;

			case ResponseCode::DisplayText:
				// TODO buffer overflow protection
				char response[512];
				inPacket >> response;
				std::cout << "Text from server: " << response << std::endl;
			}
		}
		else {
			std::cout << "Discarded packet: incorrect protocol" << std::endl;
		}
	}
}

// request game server location from punchthrough server
bool Client::contactPunchthroughServer(sf::IpAddress address, unsigned short port, int timeout_ms, int retries) {
	while (retries-- > 0) {
		sf::Packet responsePacket;
		responsePacket << PROTOCOL_VERSION << (unsigned char) 'c'; // this is a game client
		responsePacket << address.toInteger() << port;

		if (connectionSocket.send(responsePacket, punchthrough_address, punchthrough_port) != sf::Socket::Done) {
			std::cout << "could not send packet to PTS!" << std::endl;
			return false;
		}

		sf::Packet inPacket;
		if (!receiveWithTimeout(connectionSocket, inPacket, punchthrough_address, punchthrough_port, timeout_ms, "client connecting to PTS")) {
			std::cout << "PTS timed out!" << std::endl;
			return false;
		}

		sf::Uint8 protocol;
		bool response;
		if (inPacket >> protocol >> response) {
			if (response == true) {
				sf::Uint32 receivedAddress;
				sf::Uint16 receivedPort;
				if (inPacket >> receivedAddress >> receivedPort) {
					sf::IpAddress parsedAddress = sf::IpAddress(receivedAddress);

					serverAddress = parsedAddress;
					serverPort = receivedPort;
					std::cout << "server address received." << std::endl;
					return true;
				}

				continue;
			}
			else {
				std::cout << "PTS had no entry for given address." << std::endl;
				return false;
			}
		}
	}

	return false;
}

void Client::pongServer(bool isResponse) {
	sf::Packet pongPacket;
	pongPacket << PROTOCOL_VERSION << RequestCode::Pong;

	if (send(serverAddress, serverPort, pongPacket)) {
		std::cout << "ponged server" << (isResponse ? " by request." : ".") << std::endl;
	}
}

bool Client::receive(sf::IpAddress &address, unsigned short &port, sf::Packet &packet) {
	if (connectionSocket.receive(packet, address, port) != sf::Socket::Done)
		return false;
	//std::cout << "received " << packet.getDataSize() << " bytes from " << address << ":" << port << std::endl;
	return true;
}

bool Client::send(sf::IpAddress address, unsigned short port, sf::Packet &packet) {
	if (connectionSocket.send(packet, address, port) != sf::Socket::Done){
		std::cout << "serverSocket send error!" << std::endl;
		return false;
	}
	//std::cout << "sent " << packet.getDataSize() << " bytes to " << address << ":" << port << std::endl;
	return true;
}

sf::Uint32 Client::getClientID() {
	return clientID;
}