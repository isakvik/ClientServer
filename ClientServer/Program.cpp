#include <iostream>
#include <thread>
#include <future>
#include <stdlib.h>

#include <SFML/Network.hpp>
#include <SFML/System.hpp>

#include "Client.h"
#include "Constants.h"
#include "Connection.h"
#include "Server.h"
#include "Utils.h"

using namespace Network;

void runServer(unsigned short port, bool isLAN);
void runClient(sf::IpAddress server, unsigned short port, bool isLAN);

int main() {
	// Client or server ?
	char who;
	std::cout << "server (s) or client (c): ";
	std::cin >> who;

	char public_c;
	std::cout << "public connection? (y/n) ";
	std::cin >> public_c;
	bool isPublic = public_c == 'y';

	unsigned short port = 31337;

	if (who == 's') {
		std::string parse;
		std::cout << "port to use: ";
		std::cin >> parse;
		port = std::stoi(parse);

		std::cout << "running server at port " << port << ":" << std::endl;
		while (true) {
			runServer(port, isPublic);
			system("pause");
		}
	}
	else if (who == 'c') {
		char str_address[128];
		std::cout << "address to connect to: ";
		std::cin >> str_address;
		sf::IpAddress ad(str_address);

		std::string parse;
		std::cout << "port to connect to: ";
		std::cin >> parse;
		port = std::stoi(parse);

		std::cout << "running client, connecting to " << str_address << ":" << port << "..." << std::endl;
		while (true) {
			runClient(ad, port, isPublic);
			system("pause");
		}
	}

	return 0;
}

void runServer(unsigned short port, bool isPublic) {
	
	Server server(port);
	// TODO: catch fail to bind exception

	if (isPublic) {
		if (!server.contactPunchthroughServer())
			return;
	}

	

	while (true) {
		sf::Packet inPacket;
		sf::IpAddress sender;
		unsigned short senderPort;

		if (server.receive(sender, senderPort, inPacket)) {
			sf::Uint8 protocol;
			if (inPacket >> protocol) {
				if (protocol == PROTOCOL_VERSION) {
					server.processMessage(sender, senderPort, inPacket);
				}
				else {
					std::cout << "Discarded packet: incorrect protocol" << std::endl;
				}
			}
		}

		server.updatePunchthroughServerState();
		server.updateConnections();

		sleep_millis(33);
	}
}

void runClient(sf::IpAddress serverAddress, unsigned short serverPort, bool isPublic) {

	Client client;
	std::cout << "client started." << std::endl;

	if (isPublic) {
		std::cout << "connecting to punchthrough server... ";
		if (!client.contactPunchthroughServer(serverAddress, serverPort))
			return;
	}
	else {
		client.serverAddress = serverAddress;
		client.serverPort = serverPort;
	}
		
	std::cout << "connecting to " << serverAddress << ":" << serverPort << "..." << std::endl;
	client.requestServerConnection(isPublic);

	if (!client.connected) {
		return;
	}

	bool gc = true;
	while (true) {
		/* asynchronous text input loop */

		using namespace std::literals;
		auto f = std::async(std::launch::async, [] {
			auto s = ""s;
			if (std::getline(std::cin, s)) return s;
		});

		while (f.wait_for(33ms) != std::future_status::ready) {
		/**/
			sf::Packet inPacket;
			sf::IpAddress sender;
			unsigned short senderPort;

			if (client.receive(sender, senderPort, inPacket)) {
				client.processResponse(sender, senderPort, inPacket);
			}
		}

		if (!gc) { // skip garbage printing from getline() on first connect
			std::string result = f.get();
			sf::Packet outPacket;
			outPacket << PROTOCOL_VERSION << RequestCode::BroadcastMessage << client.getClientID() << result.c_str();
			client.send(client.serverAddress, client.serverPort, outPacket);
		}
		else gc = false;
		/**/
	}
}