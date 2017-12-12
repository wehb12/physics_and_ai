#include "NetworkEntity.h"

using namespace std;

void PopulateEdgeList(bool* edgeList, GraphEdge* edges, int numEdges);

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

			//vector<GraphEdge> walls;
			//for (int i = 0; i < possibleWalls; ++i)
			//{
			//	if (!maze->allEdges[i]._iswall)
			//		continue;
			//	else
			//		walls.push_back(maze->allEdges[i]);
			//}

			// mazeSize of 11 or lower can fit in an enet_unit8
			if (possibleWalls < 256)
			{
				//int numWalls = walls.size();
				MazeDataPacket8* returnPacket = new MazeDataPacket8(possibleWalls);
				//PopulateNodeLists<enet_uint8>(returnPacket->nodeA, returnPacket->nodeB, walls, maze, mazeSize * mazeSize);
				PopulateEdgeList(returnPacket->edgesThatAreWalls, maze->allEdges, possibleWalls);

				BroadcastPacket(returnPacket);
				delete returnPacket;
			}
			// maxSize of 181 or lower can fit in an enet_uint16
			else if (possibleWalls < 65536)
			{
				//int numWalls = walls.size();
				MazeDataPacket16* returnPacket = new MazeDataPacket16(possibleWalls);
				//PopulateNodeLists<enet_uint8>(returnPacket->nodeA, returnPacket->nodeB, walls, maze, mazeSize * mazeSize);
				PopulateEdgeList(returnPacket->edgesThatAreWalls, maze->allEdges, possibleWalls);

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

				//auto FindEdge = [&](int index)
				//{
				//	enet_uint8 indexA = dataPacket->nodeA[index];
				//	enet_uint8 indexB = dataPacket->nodeB[index];

				//	for (int i = 0; i < 2 * mazeSize * (mazeSize + 1); ++i)
				//	{
				//		if (maze->allEdges[i]._a->_pos == maze->allNodes[indexA]._pos)
				//		{
				//			if (maze->allEdges[i]._b->_pos == maze->allNodes[indexB]._pos)
				//				return i;
				//		}
				//		else if (maze->allEdges[i]._a->_pos == maze->allNodes[indexB]._pos)
				//		{
				//			if (maze->allEdges[i]._b->_pos == maze->allNodes[indexA]._pos)
				//				return i;
				//		}
				//	}

				//	return -1;
				//};

				for (int i = 0; i < dataPacket->numEdges; ++i)
				{
					//int index = FindEdge(i);
					if (dataPacket->edgesThatAreWalls[i])
						maze->allEdges[i]._iswall = true;
				}

				SAFE_DELETE(mazeRender);
				mazeRender = new MazeRenderer(maze);

				SceneManager::Instance()->GetCurrentScene()->AddGameObject(mazeRender);
			}
			else
				NCLLOG("No maze request data present to reconstruct from");

		}
		case MAZE_DATA16:
		{
			if (mazeSize)
			{
				MazeDataPacket16* dataPacket = new MazeDataPacket16(packet->data);

				SAFE_DELETE(maze);

				maze = new MazeGenerator();
				maze->Generate(mazeSize, 0.0f);


				for (int i = 0; i < dataPacket->numEdges; ++i)
				{
					if (dataPacket->edgesThatAreWalls[i])
						maze->allEdges[i]._iswall = true;
				}

				SceneManager::Instance()->GetCurrentScene()->RemoveGameObject(mazeRender);
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
	ENetPacket* enetPacket = enet_packet_create(stream, packet->size, 0);
	enet_peer_send(destination, 0, enetPacket);

	delete[] stream;
}

void NetworkEntity::BroadcastPacket(Packet* packet)
{
	enet_uint8* stream = packet->CreateByteStream();

	//Create the packet and broadcast it (unreliable transport) to all entitites
	ENetPacket* enetPacket = enet_packet_create(stream, packet->size, 0);
	enet_host_broadcast(networkHost, 0, enetPacket);

	delete[] stream;
}

void PopulateEdgeList(bool* edgeList, GraphEdge* edges, int numEdges)
{
	for (int i = 0; i < numEdges; ++i)
	{
		if (edges[i]._iswall)
			edgeList[i] = true;
		else
			edgeList[i] = false;
	}
}