#include "Server.h"

ConnectedClient* Server::GetClient(ENetPeer* address)
{
	for (auto it = clients.begin(); it != clients.end(); ++it)
	{
		if ((*it)->address == address)
			return *it;
	}
	return NULL;
}

ConnectedClient* Server::CreateClient(ENetPeer* address)
{
	ConnectedClient* newClient = new ConnectedClient;
	newClient->Init();

	newClient->address = address;

	AddClient(newClient);

	return newClient;
}

void Server::AddClientPositons(ENetPeer* address, Vector3 start, Vector3 end)
{
	ConnectedClient* thisClient = GetClient(address);

	thisClient->start = start;
	thisClient->end = end;
}

void Server::AddClientPath(ENetPeer* address, SearchAStar* path)
{
	ConnectedClient* thisClient = GetClient(address);

	thisClient->path = path;
}

void Server::RemoveClient(ENetPeer* address)
{
	for (auto it = clients.begin(); it != clients.end(); ++it)
	{
		if ((*it)->address == address)
		{
			clients.erase(it);
			SAFE_DELETE((*it)->path);
			(*it)->path = NULL;
			SAFE_DELETE(*it);
		}
	}
}

void Server::CreateNewMaze(int size, float density)
{
	SAFE_DELETE(maze);
	maze = new MazeGenerator;
	maze->Generate(size, density);
}

void Server::PopulateEdgeList(bool* edgeList)
{
	int numEdges = 2 * maze->size * (maze->size - 1);
	for (int i = 0; i < numEdges; ++i)
	{
		if (maze->allEdges[i]._iswall)
			edgeList[i] = true;
		else
			edgeList[i] = false;
	}
}

void Server::SetCurrentSender(ENetPeer* address)
{
	ConnectedClient* clientFound = GetClient(address);
	if (!clientFound)
		clientFound = CreateClient(address);

	currentLink = clientFound;
}

void Server::UpdateMazePositions(int indexStart, int indexEnd)
{
	maze->start = &maze->allNodes[indexStart];
	maze->end = &maze->allNodes[indexEnd];
}

void Server::UpdateClientPath()
{
	SearchAStar* aStarSearch = currentLink->path;
	aStarSearch->FindBestPath(maze->start, maze->end);

	const std::list<const GraphNode*> path = aStarSearch->GetFinalPath();
	currentLink->pathLength = path.size();

	if (currentLink->pathIndices)
	{
		delete[] currentLink->pathIndices;
		currentLink->pathIndices = NULL;
	}
	currentLink->pathIndices = new int[currentLink->pathLength];

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
		currentLink->pathIndices[i] = FindNode(*it);
		++i;
	}
}