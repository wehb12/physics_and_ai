#include "NetworkEntity.h"

using namespace std;

// templated to either enet_uint8, enet_uint16 or enet_unit32
template<typename enet_uint>
void PopulateNodeLists(enet_uint* nodeA, enet_uint* nodeB, vector<GraphEdge> walls, MazeGenerator* maze, int numNodes);

string NetworkEntity::HandlePacket(const ENetPacket* packet)
{
	enet_uint8 type = *packet->data;
	int size = sizeof(packet->data);

	string output = "";

	switch (type)
	{
		case MAZE_REQUEST:
		{
			enet_uint8 mazeSize = *(packet->data + 1);
			float mazeDensity = (float)*(packet->data + 2) / (float)255;
			output = "Create a maze of size " + to_string((int)mazeSize) + " with a density of " + to_string(mazeDensity);

			MazeGenerator* maze = new MazeGenerator;
			maze->Generate(mazeSize, mazeDensity);

			int possibleWalls = 2 * mazeSize * (mazeSize - 1);

			vector<GraphEdge> walls;
			for (int i = 0; i < possibleWalls; ++i)
			{
				if (!maze->allEdges[i]._iswall)
					continue;
				else
					walls.push_back(maze->allEdges[i]);
			}

			if (mazeSize <= 16)
			{
				int numWalls = walls.size();
				MazeDataPacket8* returnPacket = new MazeDataPacket8(numWalls);
				PopulateNodeLists<enet_uint8>(returnPacket->nodeA, returnPacket->nodeB, walls, maze, mazeSize * mazeSize);

				BroadcastPacket(returnPacket);
				delete returnPacket;
			}

			delete maze;
			break;
		}
		case MAZE_DATA8:
		{
			if (mazeSize)
			{
				MazeDataPacket8* dataPacket = new MazeDataPacket8(packet->data);

				SAFE_DELETE(maze);

				maze = new MazeGenerator();
				maze->Generate(mazeSize, 0.0f);

				auto FindEdge = [&](int index)
				{
					enet_uint8 indexA = dataPacket->nodeA[index];
					enet_uint8 indexB = dataPacket->nodeB[index];

					for (int i = 0; i < 2 * mazeSize * (mazeSize + 1); ++i)
					{
						if (maze->allEdges[i]._a->_pos == maze->allNodes[indexA]._pos)
						{
							if (maze->allEdges[i]._b->_pos == maze->allNodes[indexB]._pos)
								return i;
						}
						else if (maze->allEdges[i]._a->_pos == maze->allNodes[indexB]._pos)
						{
							if (maze->allEdges[i]._b->_pos == maze->allNodes[indexA]._pos)
								return i;
						}
					}

					return -1;
				};

				for (int i = 0; i < dataPacket->numWalls; ++i)
				{
					int index = FindEdge(i);
					if (index > 0)
						maze->allEdges[FindEdge(i)]._iswall = true;
				}

				SAFE_DELETE(mazeRender);
				mazeRender = new MazeRenderer(maze);

				SceneManager::Instance()->GetCurrentScene()->AddGameObject(mazeRender);
			}
			else
				NCLLOG("No maze request data present to reconstruct from");

		}
		default:
		{
			cout << "ERROR - Invalid packet sent" << endl;
			break;
		}
	}

	return output;
}

void NetworkEntity::SendPacket(ENetPeer* destination, Packet* packet)
{
	if (packet->type == MAZE_REQUEST)
		mazeSize = dynamic_cast<MazeRequestPacket*>(packet)->mazeSize;

	enet_uint8* stream = packet->CreateByteStream();

	//Create the packet and send it (unreliable transport) to server
	ENetPacket* enetPacket = enet_packet_create(stream, sizeof(stream), 0);
	enet_peer_send(destination, 0, enetPacket);

	delete stream;
}

void NetworkEntity::BroadcastPacket(Packet* packet)
{
	enet_uint8* stream = packet->CreateByteStream();

	//Create the packet and broadcast it (unreliable transport) to all entitites
	ENetPacket* enetPacket = enet_packet_create(stream, packet->size, 0);
	enet_host_broadcast(networkHost, 0, enetPacket);

	delete stream;
}

// templated to either enet_uint8, enet_uint16 or enet_unit32
template<typename enet_uint>
void PopulateNodeLists(enet_uint* nodeA, enet_uint* nodeB, vector<GraphEdge> walls, MazeGenerator* maze, int numNodes)
{
	int size = walls.size();

	if (size == 0)
		return;

	auto FindNode = [&](GraphNode* node)
	{
		for (enet_uint i = 0; i < numNodes; ++i)
		{
			if (&maze->allNodes[i] == node)
				return i;
		}
	};

	for (int i = 0; i < size; ++i)
	{
		nodeA[i] = FindNode(walls[i]._a);
		nodeB[i] = FindNode(walls[i]._b);
	}
}