#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "BitStream.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <map>

static unsigned int SERVER_PORT = 65000;
static unsigned int CLIENT_PORT = 65001;
static unsigned int MAX_CONNECTIONS = 4;

enum NetworkState
{
	NS_Init = 0,
	NS_PendingStart,
	NS_Started,
	NS_Lobby,
	NS_Pending,
	NS_CharacterSelect,
	NS_Game
};

bool isServer = false;
bool isRunning = true;

RakNet::RakPeerInterface *g_rakPeerInterface = nullptr;
RakNet::SystemAddress g_serverAddress;

NetworkState g_networkState = NS_Init;

enum {
	ID_THEGAME_LOBBY_READY = ID_USER_PACKET_ENUM,
	ID_THEGAME_START,
};

struct SPlayer
{
	std::string name;
	std::string health;
	std::string profession;
	RakNet::SystemAddress address;
};

std::map<unsigned long, SPlayer> m_players;

//server
void OnIncomingConnection(RakNet::Packet* packet)
{
	//must be server in order to recieve connection
	assert(isServer);
	m_players.insert(std::make_pair(RakNet::RakNetGUID::ToUint32(packet->guid), SPlayer()));
	std::cout << "Total Players: " << m_players.size() << std::endl;
}

//client
void OnConnectionAccepted(RakNet::Packet* packet)
{
	//server should not be connecting to anybody, 
	//clients connect to server
	assert(!isServer);

	g_networkState = NS_Lobby;
	g_serverAddress = packet->systemAddress;
}

void OnLobbyReady(RakNet::Packet* packet)
{
	unsigned long guid = RakNet::RakNetGUID::ToUint32(packet->guid);
	std::map<unsigned long, SPlayer>::iterator it = m_players.find(guid);
	//somehow player didn't connect but now is in lobby ready
	assert(it != m_players.end());

	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	RakNet::RakString userName;
	bs.Read(userName);

	SPlayer& player = it->second;
	player.name = userName;
	std::cout << userName.C_String() << " aka " << player.name.c_str() << " IS READY!!!!!" << std::endl;

}

//void sendPacketsToClients(RakNet::MessageID id)
//{
//	//send packet back
//	std::string nullIn = "null";
//	RakNet::BitStream myBitStream;
//	//first thing to write, is packet message identifier
//	myBitStream.Write(id);
//	RakNet::RakString name(nullIn.c_str());
//	myBitStream.Write(name);
//
//	for (auto const& x : m_playerMap)
//	{
//		g_rakPeerInterface->Send(&myBitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, x.second.address, false);
//	}
//
//}

unsigned char GetPacketIdentifier(RakNet::Packet *packet)
{
	if (packet == nullptr)
		return 255;

	if ((unsigned char)packet->data[0] == ID_TIMESTAMP)
	{
		RakAssert(packet->length > sizeof(RakNet::MessageID) + sizeof(RakNet::Time));
		return (unsigned char)packet->data[sizeof(RakNet::MessageID) + sizeof(RakNet::Time)];
	}
	else
		return (unsigned char)packet->data[0];
}


void InputHandler()
{
	while (isRunning)
	{
		char userInput[255];
		if (g_networkState == NS_Init)
		{
			std::cout << "press (s) for server (c) for client" << std::endl;
			std::cin >> userInput;
			isServer = (userInput[0] == 's');
			g_networkState = NS_PendingStart;
		}
		else if (g_networkState == NS_Lobby)
		{
			std::cout << "Enter your name to play or type quit to leave" << std::endl;
			std::cin >> userInput;
			//quitting is not acceptable in our game, create a crash to teach lesson
			assert(strcmp(userInput, "quit"));

			RakNet::BitStream bs;
			bs.Write((RakNet::MessageID)ID_THEGAME_LOBBY_READY);
			RakNet::RakString name(userInput);
			bs.Write(name);

			//returns 0 when something is wrong
			assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false));
			g_networkState = NS_Pending;
		}
		else if (g_networkState == NS_Pending)
		{
			std::cout << "pending..." << std::endl;
		}
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}
}

bool HandleLowLevelPackets(RakNet::Packet* packet)
{
	bool isHandled = true;
	// We got a packet, get the identifier with our handy function
	unsigned char packetIdentifier = GetPacketIdentifier(packet);

	// Check if this is a network message packet
	switch (packetIdentifier)
	{
	case ID_DISCONNECTION_NOTIFICATION:
		// Connection lost normally
		printf("ID_DISCONNECTION_NOTIFICATION\n");
		break;
	case ID_ALREADY_CONNECTED:
		// Connection lost normally
		printf("ID_ALREADY_CONNECTED with guid %" PRINTF_64_BIT_MODIFIER "u\n", packet->guid);
		break;
	case ID_INCOMPATIBLE_PROTOCOL_VERSION:
		printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
		break;
	case ID_REMOTE_DISCONNECTION_NOTIFICATION: // Server telling the clients of another client disconnecting gracefully.  You can manually broadcast this in a peer to peer enviroment if you want.
		printf("ID_REMOTE_DISCONNECTION_NOTIFICATION\n");
		break;
	case ID_REMOTE_CONNECTION_LOST: // Server telling the clients of another client disconnecting forcefully.  You can manually broadcast this in a peer to peer enviroment if you want.
		printf("ID_REMOTE_CONNECTION_LOST\n");
		break;
	case ID_NEW_INCOMING_CONNECTION:
		OnIncomingConnection(packet);
		printf("ID_NEW_INCOMING_CONNECTION\n");
		break;
	case ID_REMOTE_NEW_INCOMING_CONNECTION: // Server telling the clients of another client connecting.  You can manually broadcast this in a peer to peer enviroment if you want.
		OnIncomingConnection(packet);
		printf("ID_REMOTE_NEW_INCOMING_CONNECTION\n");
		break;
	case ID_CONNECTION_BANNED: // Banned from this server
		printf("We are banned from this server.\n");
		break;
	case ID_CONNECTION_ATTEMPT_FAILED:
		printf("Connection attempt failed\n");
		break;
	case ID_NO_FREE_INCOMING_CONNECTIONS:
		// Sorry, the server is full.  I don't do anything here but
		// A real app should tell the user
		printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
		break;

	case ID_INVALID_PASSWORD:
		printf("ID_INVALID_PASSWORD\n");
		break;

	case ID_CONNECTION_LOST:
		// Couldn't deliver a reliable packet - i.e. the other system was abnormally
		// terminated
		printf("ID_CONNECTION_LOST\n");
		break;

	case ID_CONNECTION_REQUEST_ACCEPTED:
		// This tells the client they have connected
		printf("ID_CONNECTION_REQUEST_ACCEPTED to %s with GUID %s\n", packet->systemAddress.ToString(true), packet->guid.ToString());
		printf("My external address is %s\n", g_rakPeerInterface->GetExternalID(packet->systemAddress).ToString(true));
		OnConnectionAccepted(packet);
		break;
	case ID_CONNECTED_PING:
	case ID_UNCONNECTED_PING:
		printf("Ping from %s\n", packet->systemAddress.ToString(true));
		break;
	default:
		isHandled = false;
		break;
	}
	return isHandled;
}

void PacketHandler()
{
	while (isRunning)
	{
		for (RakNet::Packet* packet = g_rakPeerInterface->Receive(); packet != nullptr; g_rakPeerInterface->DeallocatePacket(packet), packet = g_rakPeerInterface->Receive())
		{
			if (!HandleLowLevelPackets(packet))
			{
				//our game specific packets
				unsigned char packetIdentifier = GetPacketIdentifier(packet);
				switch (packetIdentifier)
				{
				case ID_THEGAME_LOBBY_READY:
					OnLobbyReady(packet);
					break;
				default:
					break;
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}


}

int main()
{
	g_rakPeerInterface = RakNet::RakPeerInterface::GetInstance();

	std::thread inputHandler(InputHandler);
	std::thread packetHandler(PacketHandler);

	while (isRunning)
	{
		if (g_networkState == NS_PendingStart)
		{
			if (isServer)
			{
				RakNet::SocketDescriptor socketDescriptors[1];
				socketDescriptors[0].port = SERVER_PORT;
				socketDescriptors[0].socketFamily = AF_INET; // Test out IPV4

				bool isSuccess = g_rakPeerInterface->Startup(MAX_CONNECTIONS, socketDescriptors, 1) == RakNet::RAKNET_STARTED;
				assert(isSuccess);
				//ensures we are server
				g_rakPeerInterface->SetMaximumIncomingConnections(MAX_CONNECTIONS);
				std::cout << "server started" << std::endl;
				g_networkState = NS_Started;
			}
			//client
			else
			{
				RakNet::SocketDescriptor socketDescriptor(CLIENT_PORT, 0);
				socketDescriptor.socketFamily = AF_INET;

				while (RakNet::IRNS2_Berkley::IsPortInUse(socketDescriptor.port, socketDescriptor.hostAddress, socketDescriptor.socketFamily, SOCK_DGRAM) == true)
					socketDescriptor.port++;

				RakNet::StartupResult result = g_rakPeerInterface->Startup(8, &socketDescriptor, 1);
				assert(result == RakNet::RAKNET_STARTED);
				g_rakPeerInterface->SetOccasionalPing(true);
				//"127.0.0.1" = local host = your machines address
				RakNet::ConnectionAttemptResult car = g_rakPeerInterface->Connect("127.0.0.1", SERVER_PORT, nullptr, 0);
				RakAssert(car == RakNet::CONNECTION_ATTEMPT_STARTED);
				std::cout << "client attempted connection..." << std::endl;
				g_networkState = NS_Started;
			}
		}

	}

	//std::cout << "press q and then return to exit" << std::endl;
	//std::cin >> userInput;

	inputHandler.join();
	packetHandler.join();
	return 0;
}