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
			mazeSize = dataPacket->mazeSize;
			mazeDensity = (float)(dataPacket->mazeDensity) / (float)(MAX_VAL_8BIT);
			output = "Create a maze of size " + to_string(mazeSize) + " with a density of " + to_string(mazeDensity);

			printPath = false;
			SAFE_DELETE(maze);
			maze = new MazeGenerator;
			maze->Generate(mazeSize, mazeDensity);

			int possibleWalls = 2 * mazeSize * (mazeSize - 1);

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

			delete dataPacket;
			break;
		}
		case MAZE_DATA8:
		{
			if (mazeSize)
			{
				MazeDataPacket8* dataPacket = new MazeDataPacket8(packet->data);

				printPath = false;
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

				printPath = false;
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
			HandlePositionPacket(posPacket);
			delete posPacket;
			break;
		}
		case MAZE_POSITIONS16:
		{
			MazePositionsPacket16* posPacket = new MazePositionsPacket16(packet->data);
			HandlePositionPacket(posPacket);
			delete posPacket;
			break;
		}
		case MAZE_PATH8:
		{
			MazePathPacket8* pathPacket = new MazePathPacket8(packet->data);
			PopulatePath(pathPacket);
			delete pathPacket;
			break;
		}
		case MAZE_PATH16:
		{
			MazePathPacket16* pathPacket = new MazePathPacket16(packet->data);
			PopulatePath(pathPacket);
			delete pathPacket;
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

template <class PositionPacket>
void NetworkEntity::HandlePositionPacket(PositionPacket* posPacket)
{
	*maze->start = maze->allNodes[posPacket->start];
	*maze->end = maze->allNodes[posPacket->end];

	aStarSearch->FindBestPath(maze->start, maze->end);

	const std::list<const GraphNode*> path = aStarSearch->GetFinalPath();

	int* pathIndices = new int[path.size()];

	// lambda function takes in a node and returns the index into the array
	auto FindNode = [&](const GraphNode* node)
	{
		Vector3 posToFind = node->_pos;

		for (int i = 0; i < maze->size * maze->size; ++i)
		{
			if (maze->allNodes[i]._pos == posToFind)
				return i;
		}
		return -1;
	};

	int i = 0;
	for (auto it = path.begin(); it != path.end(); ++it)
	{
		pathIndices[i] = FindNode(*it);
		++i;
	}

	if ((maze->size * maze->size < MAX_VAL_8BIT) && (path.size() < MAX_VAL_8BIT))
	{
		MazePathPacket8* pathPacket = new MazePathPacket8(pathIndices, path.size());
		SendPacket(currentPacketSender, pathPacket);
		delete pathPacket;
	}
	else if ((maze->size * maze->size < MAX_VAL_16BIT) && (path.size() < MAX_VAL_16BIT))
	{
		MazePathPacket16* pathPacket = new MazePathPacket16(pathIndices, path.size());
		SendPacket(currentPacketSender, pathPacket);
		delete pathPacket;
	}
}

template <class PathPacket>
void NetworkEntity::PopulatePath(PathPacket* pathPacket)
{
	pathLength = pathPacket->length;
	if (path)
	{
		delete[] path;
		path = NULL;
	}
	path = new int[pathLength];
	for (int i = 0; i < pathLength; ++i)
		path[i] = pathPacket->path[i];

	printPath = true;
}

void NetworkEntity::PrintPath()
{
	float grid_scalar = 1.0f / (float)maze->GetSize();
	float col_factor = 0.2f / (float)pathLength;

	Matrix4 transform = mazeRender->Render()->GetWorldTransform();

	float index = 0.0f;
	for (int i = 0; i < pathLength - 1; ++i)
	{
		Vector3 start = transform * Vector3(
			(maze->allNodes[path[i]]._pos.x + 0.5f) * grid_scalar,
			0.1f,
			(maze->allNodes[path[i]]._pos.y + 0.5f) * grid_scalar);

		Vector3 end = transform * Vector3(
			(maze->allNodes[path[i + 1]]._pos.x + 0.5f) * grid_scalar,
			0.1f,
			(maze->allNodes[path[i + 1]]._pos.y + 0.5f) * grid_scalar);

		NCLDebug::DrawThickLine(start, end, 2.5f / mazeSize, CommonUtils::GenColor(0.8f + i * col_factor));
	}
}

void NetworkEntity::SendPacket(ENetPeer* destination, Packet* packet)
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