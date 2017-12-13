#include "Server.h"
#include "Client.h"
#include "PacketHandler.h"

using namespace std;

string PacketHandler::HandlePacket(const ENetPacket* packet)
{
	enet_uint8 packetType = *packet->data;

	string output = "";

	switch (packetType)
	{
		case MESSAGE:
		{
			MessagePacket* message = new MessagePacket(packet->data);
			output = message->message;
			break;
		}
		case MAZE_REQUEST:
		{
			if (entityType == SERVER)
			{
				MazeRequestPacket* dataPacket = new MazeRequestPacket(packet->data);
				HandleMazeRequestPacket(dataPacket);

				output = "Create a maze of size " + to_string(mazeSize) + " with a density of " + to_string(mazeDensity);
				delete dataPacket;
			}
			break;
		}
		case MAZE_DATA8:
		{
			if (entityType == CLIENT)
			{
				MazeDataPacket8* dataPacket = new MazeDataPacket8(packet->data);
				output = "Construct a maze";

				Client::Instance()->SetPrintPathState(false);
				HandleMazeDataPacket(dataPacket);
				delete dataPacket;
			}
			else
				NCLLOG("No maze request data present to reconstruct from");
			break;
		}
		case MAZE_DATA16:
		{
			if (entityType == CLIENT)
			{
				MazeDataPacket16* dataPacket = new MazeDataPacket16(packet->data);
				output = "Construct a maze";

				Client::Instance()->SetPrintPathState(false);
				HandleMazeDataPacket(dataPacket);
				delete dataPacket; 
			}
			else
				NCLLOG("No maze request data present to reconstruct from");
			break;
		}
		case MAZE_POSITIONS8:
		{
			if (entityType == SERVER)
			{
				MazePositionsPacket8* posPacket = new MazePositionsPacket8(packet->data);
				output = "Here are my start and end positions.";

				HandlePositionPacket(posPacket);
				delete posPacket;
			}
			break;
		}
		case MAZE_POSITIONS16:
		{
			if (entityType == SERVER)
			{
				MazePositionsPacket16* posPacket = new MazePositionsPacket16(packet->data);
				output = "Here are my start and end positions.";

				HandlePositionPacket(posPacket);
				delete posPacket;
			}
			break;
		}
		case MAZE_PATH8:
		{
			if (entityType == CLIENT)
			{
				MazePathPacket8* pathPacket = new MazePathPacket8(packet->data);
				output = "Here is your path.";

				Client::Instance()->PopulatePath(pathPacket);
				delete pathPacket;
			}
			break;
		}
		case MAZE_PATH16:
		{
			if (entityType == CLIENT)
			{
				MazePathPacket16* pathPacket = new MazePathPacket16(packet->data);
				output = "Here is your path.";

				Client::Instance()->PopulatePath(pathPacket);
				delete pathPacket;
			}
			break;
		}
		default:
		{
			cout << "ERROR - Invalid packet sent" << endl;
			break;
		}
	}

	return output;
}

void PacketHandler::HandleMazeRequestPacket(MazeRequestPacket* reqPacket)
{
	mazeSize = reqPacket->mazeSize;
	mazeDensity = (float)(reqPacket->mazeDensity) / (float)(MAX_VAL_8BIT);

	Server::Instance()->CreateNewMaze(mazeSize, mazeDensity);

	int possibleWalls = 2 * mazeSize * (mazeSize - 1);

	// mazeSize of 11 or lower can fit in an enet_unit8
	if (possibleWalls <= MAX_VAL_8BIT)
	{
		MazeDataPacket8* returnPacket = new MazeDataPacket8(possibleWalls);
		Server::Instance()->PopulateEdgeList(returnPacket->edgesThatAreWalls);

		BroadcastPacket(returnPacket);
		delete returnPacket;
	}
	// maxSize of 181 or lower can fit in an enet_uint16
	else if (possibleWalls <= MAX_VAL_16BIT)
	{
		MazeDataPacket16* returnPacket = new MazeDataPacket16(possibleWalls);
		Server::Instance()->PopulateEdgeList(returnPacket->edgesThatAreWalls);

		BroadcastPacket(returnPacket);
		delete returnPacket;
	}
	else
		NCLERROR("Maze size too large");
}

template <class DataPacket>
void PacketHandler::HandleMazeDataPacket(DataPacket* dataPacket)
{
	Client::Instance()->GenerateWalledMaze(dataPacket->edgesThatAreWalls, dataPacket->numEdges);

	Client::Instance()->RenderNewMaze();
}

void PacketHandler::SendPositionPacket(ENetPeer* dest, MazeGenerator* maze)
{
	int numNodes = maze->size * maze->size;

	// lambda function takes in a node and returns the index into the array
	auto FindNode = [&](GraphNode* node)
	{
		Vector3 posToFind = node->_pos;

		for (int i = 0; i < numNodes; ++i)
		{
			if (maze->allNodes[i]._pos == posToFind)
				return i;
		}
		return -1;
	};

	// finmd indices into the allNodes array
	int startIndex = FindNode(maze->start);
	int endIndex = FindNode(maze->end);

	if (startIndex < 0 || endIndex < 0)
		NCLERROR("Invalid start or end position");

	// mazeSize of 16 or lower has fewer than MAX_VAL_8BIT nodes
	if (numNodes <= MAX_VAL_8BIT)
	{
		MazePositionsPacket8* posPacket = new MazePositionsPacket8(startIndex, endIndex);
		SendPacket(Client::Instance()->GetServerConnection(), posPacket);
		delete posPacket;
	}
	// mazeSize of 256 or lower has fewer than MAX_VAL_16BIT nodes
	else if (numNodes <= MAX_VAL_16BIT)
	{
		MazePositionsPacket16* posPacket = new MazePositionsPacket16(startIndex, endIndex);
		SendPacket(Client::Instance()->GetServerConnection(), posPacket);
		delete posPacket;
	}
}

template <class PositionPacket>
void PacketHandler::HandlePositionPacket(PositionPacket* posPacket)
{
	Server::Instance()->UpdateMazePositions(posPacket->start, posPacket->end);

	Server::Instance()->UpdateClientPath();

	int* pathIndices = Server::Instance()->GetPathIndices();
	int pathLength = Server::Instance()->GetPathLength();

	int mazeSize = Server::Instance()->GetMazeSize();
	int mazeSizeSquared = mazeSize * mazeSize;

	if ((mazeSizeSquared < MAX_VAL_8BIT) && (pathLength < MAX_VAL_8BIT))
	{
		MazePathPacket8* pathPacket = new MazePathPacket8(pathIndices, pathLength);
		SendPacket(Server::Instance()->GetCurrentLinkAddress(), pathPacket);
		delete pathPacket;
	}
	else if ((mazeSizeSquared < MAX_VAL_16BIT) && (pathLength < MAX_VAL_16BIT))
	{
		MazePathPacket16* pathPacket = new MazePathPacket16(pathIndices, pathLength);
		SendPacket(Server::Instance()->GetCurrentLinkAddress(), pathPacket);
		delete pathPacket;
	}
}

void PacketHandler::SendPacket(ENetPeer* destination, Packet* packet)
{
	if (packet->type == MAZE_REQUEST)
	{
		MazeRequestPacket* requestPacket = dynamic_cast<MazeRequestPacket*>(packet);
		mazeSize = requestPacket->mazeSize;
		mazeDensity = (float)(requestPacket->mazeDensity) / MAX_VAL_8BIT;
	}

	enet_uint8* stream = packet->CreateByteStream();

	//Create the packet and send it (unreliable transport) to server
	ENetPacket* enetPacket = enet_packet_create(stream, packet->size, 0);
	enet_peer_send(destination, 0, enetPacket);

	delete[] stream;
}

void PacketHandler::BroadcastPacket(Packet* packet)
{
	enet_uint8* stream = packet->CreateByteStream();

	//Create the packet and broadcast it (unreliable transport) to all entitites
	ENetPacket* enetPacket = enet_packet_create(stream, packet->size, 0);
	enet_host_broadcast(networkHost, 0, enetPacket);

	delete[] stream;
}