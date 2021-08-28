#pragma once

#include <SFML/Network.hpp>
#include <SFML/System.hpp>

namespace Network {
	typedef unsigned char byte;

	const sf::Uint8			PROTOCOL_VERSION = 0x7F;
	const sf::IpAddress		punchthrough_address("");
	const unsigned short	punchthrough_port = 12199;

	const unsigned int		FNV1A_PRIME = 0x01000193; //   16777619 
	const unsigned int		FNV1A_SEED = 0x811C9DC5;

	////////////// client request/server response codes below

	enum RequestCode : sf::Uint8 {
		Pong = 0,
		RequestConnection = 1,
		RequestConnectionWithPassword = 15,

		VerifyConnection = 16,
		RequestDisconnection = 17,
		BroadcastMessage = 18,
		UpdatePlayer = 19,

		SyncClocks = 24,

		PTSNotification = 255
	};
	
	enum ResponseCode : sf::Uint8 {
		Ping = 0,
		ConnectionReceived = 1,
		ConnectionDenied = 2,
		ConnectionRequired = 3,
		ForciblyDisconnected = 4,
		ErroneousRequest = 5,

		PasswordRequired = 15,
		DisplayText = 16,			// treat as system message

		SyncClocksResponse = 24
	};

	// for easily putting codes into packets as Uint8 types
	inline
	sf::Packet& operator <<(sf::Packet& packet, const RequestCode& requestCode) {
		return packet << (sf::Uint8) requestCode;
	}

	inline
	sf::Packet& operator <<(sf::Packet& packet, const ResponseCode& responseCode) {
		return packet << (sf::Uint8) responseCode;
	}
}