
#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "BitStream.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <map>

static int SERVER_PORT = 65000;
static int CLIENT_PORT = 65001;
static int MAX_CONNECTIONS = 4;

RakNet::RakPeerInterface *g_rakPeerInterface = nullptr;

unsigned char GetPacketIdentifier(RakNet::Packet *packet);
unsigned char packetIdentifier;

bool isServer = false;
bool isRunning = true;
unsigned short g_totalPlayers = 0;

enum {
	ID_THEGAME_LOBBY = ID_USER_PACKET_ENUM,
	ID_THEGAME_ACTION, 
};

struct SPlayer
{
	//struct for declaring player related things like name, health
	std::string name;
	RakNet::SystemAddress address;
	//state
	
};

RakNet::SystemAddress g_serverAddress;

std::map<unsigned long, SPlayer> m_playerMap;


enum NetworkStates
{
	NS_Decision = 0,
	NS_CreateSocket,
	NS_PendingConnection,
	NS_Connected,
	NS_Running,
	NS_Lobby,
};

NetworkStates g_networkState = NS_Decision;

void OnConnectionAccepted(RakNet::Packet* packet)
{
	if (isServer)
	{
		//server should never request connections, only clients do
		assert(0);
	}
	//we have successfully connected, go to lobby
	g_networkState = NS_Lobby;
	g_serverAddress = packet->systemAddress;
}

void OnIncomingConnection(RakNet::Packet* packet)
{
	if (!isServer)
	{
		assert(0);
	}
	g_totalPlayers++;
	unsigned short numConnections = g_rakPeerInterface->NumberOfConnections();
	std::cout << "Total Players: " << g_totalPlayers << "Num Connection: " << numConnections << std::endl;
}

void InputHandler()
{
	while (isRunning)
	{
		char userInput[255];
		if (g_networkState == NS_Decision)
		{
		
		std::cout << "Press (s) for server, (c) for client" << std::endl;
		std::cin >> userInput;
		isServer = userInput[0] == 's';
		g_networkState = NS_CreateSocket;
		}
		else if (g_networkState == NS_CreateSocket)
		{
			if (isServer)
			{
				std::cout << "Server creating socket..." << std::endl;
			}
			else
			{
				std::cout << "Client creating socket..." << std::endl;
			}
		}
		else if (g_networkState == NS_Lobby)
		{
			std::cout << "If you would like to play this game, enter your player name" << std::endl;
			std::cout << "If you would like to quit, type quit." << std::endl;
			std::cin >> userInput;
			if (strcmp(userInput, "quit") == 0)
			{
				//heartbreaking
				assert(0);
			}
			else
			{
				//send our first packet
				RakNet::BitStream myBitStream;
				//First thing to write, is packet message identifier
				myBitStream.Write((RakNet::MessageID)ID_THEGAME_LOBBY);
				RakNet::RakString name(userInput);
				myBitStream.Write(name);

				g_rakPeerInterface->Send(&myBitStream,HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false);
			}
		}

		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}
}

unsigned char GetPacketIdentifier(RakNet::Packet *packet)
{
	if (packet == 0)
		return 255;

	if ((unsigned char)packet->data[0] == ID_TIMESTAMP)
	{
		RakAssert(packet->length > sizeof(RakNet::MessageID) + sizeof(RakNet::Time));
		return (unsigned char)packet->data[sizeof(RakNet::MessageID) + sizeof(RakNet::Time)];
	}
	else
		return (unsigned char)packet->data[0];
}

bool HandleLowLevelPacket(RakNet::Packet* packet)
{
	bool isHandled = true;
	unsigned char packetIdentifier = GetPacketIdentifier(packet);
	switch ()
	{

	}
}

void PacketHandler()
{
	while (isRunning)
	{
		for (RakNet::Packet* packet = g_rakPeerInterface->Receive(); packet != nullptr; g_rakPeerInterface->DeallocatePacket(packet), packet = g_rakPeerInterface->Receive())
		{
			// We got a packet, get the identifier with our handy function
			packetIdentifier = GetPacketIdentifier(packet);

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
			case ID_REMOTE_NEW_INCOMING_CONNECTION: // Server telling the clients of another client connecting.  You can manually broadcast this in a peer to peer enviroment if you want.
				printf("ID_REMOTE_NEW_INCOMING_CONNECTION\n");
				OnIncomingConnection(packet);
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
				// It's a client, so just show the message
				printf("%s\n", packet->data);
				break;
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
		if (g_networkState == NS_CreateSocket)
		{
			if (isServer)
			{
				//opening up the server socket
				RakNet::SocketDescriptor socketDescriptors[1];
				socketDescriptors[0].port = SERVER_PORT;
				socketDescriptors[0].socketFamily = AF_INET; // Test out IPV4
				bool isSuccess = g_rakPeerInterface->Startup(MAX_CONNECTIONS, socketDescriptors, 1) == RakNet::RAKNET_STARTED;
				assert(isSuccess);
				g_rakPeerInterface->SetMaximumIncomingConnections(MAX_CONNECTIONS);
				g_networkState = NS_PendingConnection;
				std::cout << "Server waiting on Connections..." << std::endl;

			}
			else
			{
				RakNet::SocketDescriptor socketDescriptor(CLIENT_PORT, nullptr);
				socketDescriptor.socketFamily = AF_INET;

				while (RakNet::IRNS2_Berkley::IsPortInUse(socketDescriptor.port, socketDescriptor.hostAddress, socketDescriptor.socketFamily, SOCK_DGRAM) == true)
					socketDescriptor.port++;
				
				g_rakPeerInterface->Startup(8, &socketDescriptor, 1);

				// client connection
				//127=local host = yourself
				RakNet::ConnectionAttemptResult car = g_rakPeerInterface->Connect("127.0.0.1", SERVER_PORT,nullptr,0);
				RakAssert(car == RakNet::CONNECTION_ATTEMPT_STARTED);
				std::cout << "Client attempted connection... waiting for response" << std::endl;
				g_networkState = NS_PendingConnection;
			}
		}
	}


	

	inputHandler.join();
	packetHandler.join();
	return 0;
}