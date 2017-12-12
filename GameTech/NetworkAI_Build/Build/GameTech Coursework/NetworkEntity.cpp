#include "NetworkEntity.h"

using namespace std;

void PopulateEdgeList(bool* edgeList, GraphEdge* edges, int numEdges);

string NetworkEntity::HandlePacket(const ENetPacket* packet)
{
	enet_uint8 type = *packet->data;

	string output = "";

	switch (type)
	{
		case MESSAGE:
		{
			MessagePacket* message = new MessagePacket(packet->data);
			output = message->message;
			break;
		}
		case MAZE_REQUEST:
		{
			MazeRequestPacket* dataPacket = new MazeRequestPacket(packet->data);
			output = "Create a maze of size " + to_string((int)dataPacket->mazeSize) + " with a density of " + to_string((float)(dataPacket->mazeDensity) / MAX_VAL_8BIT);

			SAFE_DELETE(maze);
			maze = new MazeGenerator;
			maze->Generate(dataPacket->mazeSize, dataPacket->mazeDensity);

			int possibleWalls = 2 * dataPacket->mazeSize * (dataPacket->mazeSize - 1);

			// mazeSize of 11 or lower can fit in an enet_unit8
			if (possibleWalls <= MAX_VAL_8BIT)
			{
				MazeDataPacket8* returnPacket = new MazeDataPacket8(possibleWalls);
				PopulateEdgeList(returnPacket->edgesThatAreWalls, maze->allEdges, possibleWalls);

				BroadcastPacket(returnPacket);
				delete returnPacket;
			}
			// maxSize of 181 or lower can fit in an enet_uint16
			else if (possibleWalls <= MAX_VAL_16BIT)
			{
				MazeDataPacket16* returnPacket = new MazeDataPacket16(possibleWalls);
				PopulateEdgeList(returnPacket->edgesThatAreWalls, maze->allEdges, possibleWalls);

				BroadcastPacket(returnPacket);
				delete returnPacket;
			}
			else
				NCLERROR("Maze size too large");

			break;
		}
		case MAZE_DATA8:
		{
			if (mazeSize)
			{
				MazeDataPacket8* dataPacket = new MazeDataPacket8(packet->data);

				HandleMazeDataPacket(dataPacket);
				delete dataPacket;
			}
			else
				NCLLOG("No maze request data present to reconstruct from");
			break;
		}
		case MAZE_DATA16:
		{
			if (mazeSize)
			{
				MazeDataPacket16* dataPacket = new MazeDataPacket16(packet->data);

				HandleMazeDataPacket(dataPacket);
				delete dataPacket;
			}
			else
				NCLLOG("No maze request data present to reconstruct from");
			break;
		}
		case MAZE_POSITIONS8:
		{
			MazePositionsPacket8* posPacket = new MazePositionsPacket8(packet->data);
			
			*maze->start = maze->allNodes[posPacket->start];
			*maze->end = maze->allNodes[posPacket->end];

			break;
		}
		case MAZE_POSITIONS16:
		{
			MazePositionsPacket16* posPacket = new MazePositionsPacket16(packet->data);

			*maze->start = maze->allNodes[posPacket->start];
			*maze->end = maze->allNodes[posPacket->end];

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

template <class DataPacket>
void NetworkEntity::HandleMazeDataPacket(DataPacket* dataPacket)
{
	SAFE_DELETE(maze);

	maze = new MazeGenerator();
	maze->Generate(mazeSize, 0.0f);


	for (int i = 0; i < dataPacket->numEdges; ++i)
	{
		if (dataPacket->edgesThatAreWalls[i])
			maze->allEdges[i]._iswall = true;
	}

	if (mazeRender)
	{
		SceneManager::Instance()->GetCurrentScene()->RemoveGameObject(mazeRender);
		delete mazeRender;
		mazeRender = NULL;
	}
	if (!wallmesh)
	{
		GLuint whitetex;
		glGenTextures(1, &whitetex);
		glBindTexture(GL_TEXTURE_2D, whitetex);
		unsigned int pixel = 0xFFFFFFFF;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, &pixel);
		glBindTexture(GL_TEXTURE_2D, 0);

		wallmesh = new OBJMesh("../../Data/Meshes/cube.obj");
		wallmesh->SetTexture(whitetex);
	}
	mazeRender = new MazeRenderer(maze, wallmesh);

	//The maze is returned in a [0,0,0] - [1,1,1] cube (with edge walls outside) regardless of grid_size,
	// so we need to scale it to whatever size we want
	Matrix4 maze_scalar = Matrix4::Scale(Vector3(5.f, 5.0f / float(mazeSize), 5.f)) * Matrix4::Translation(Vector3(-0.5f, 0.f, -0.5f));
	mazeRender->Render()->SetTransform(maze_scalar);
	SceneManager::Instance()->GetCurrentScene()->AddGameObject(mazeRender);

	// now send back to the server the start and end positions

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
		SendPacket(serverConnection, posPacket);
		delete posPacket;
	}
	// mazeSize of 256 or lower has fewer than MAX_VAL_16BIT nodes
	else if (numNodes <= MAX_VAL_16BIT)
	{
		MazePositionsPacket16* posPacket = new MazePositionsPacket16(startIndex, endIndex);
		SendPacket(serverConnection, posPacket);
		delete posPacket;
	}
}

void NetworkEntity::SendPacket(ENetPeer* destination, Packet* packet)
{
	if (packet->type == MAZE_REQUEST)
	{
		MazeRequestPacket* requestPacket = dynamic_cast<MazeRequestPacket*>(packet);
		mazeSize = requestPacket->mazeSize;
		mazeDensity = requestPacket->mazeDensity / MAX_VAL_8BIT;
	}

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