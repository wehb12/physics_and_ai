#include "PacketHandler.h"
#include "Server.h"

void Server::Update(float msec)
{
	timeElapsed += msec;

	if (timeElapsed > TIME_STEP)
	{
		if (clients.size() > 0)
			UpdateAvatarPositions(msec);

		timeElapsed = 0.0f;
	}
}

void Server::UpdateAvatarPositions(float msec)
{
	for (auto it = clients.begin(); it != clients.end(); ++it)
	{
		if ((*it)->move)
		{
			if ((*it)->timeToNext > 0)
			{
				(*it)->timeToNext -= timeElapsed;
				//(*it)->avatarPos = (*it)->avatarPos + (*it)->avatarVel;
				(*it)->avatarPos.x = (*it)->avatarPNode->GetPosition().x;
				(*it)->avatarPos.y = (*it)->avatarPNode->GetPosition().z;
			}
			else
			{
				(*it)->timeToNext = (*it)->timeStep;
				if ((*it)->currentAvatarIndex < (*it)->pathLength - 2)
				{
					++(*it)->currentAvatarIndex;
					SetAvatarVelocity(*it);
				}
				else
				{
					(*it)->move = false;
					(*it)->currentAvatarIndex = 0;
				}
			}
			TransmitAvatarPosition(*it);
		}
	}
}

void Server::InitClient(ENetPeer* address)
{
	ConnectedClient* newClient = CreateClient(address);


}

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
	++numClients;
	newClient->clientID = numClients;

	newClient->address = address;

	AddClient(newClient);

	return newClient;
}

void Server::AddClientPositons(Vector3 start, Vector3 end)
{
	currentLink->start = start;
	currentLink->end = end;
}

void Server::RemoveClient(ENetPeer* address)
{
	if (clients.size() > 0)
	{
		int i = 0;
		for (auto it = clients.begin(); it != clients.end(); ++it)
		{
			if ((*it)->address == address)
			{
				ConnectedClient* temp = (*it);
				clients.erase(it);
				SAFE_DELETE(temp);
				--numClients;
				break;
			}
			++i;
		}
		if (i < clients.size())
			for (; i < clients.size(); ++i)
				clients[i]->clientID = clients[i]->clientID - 1;
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

	AddClientPositons(maze->start->_pos, maze->end->_pos);
}

void Server::UpdateClientPath()
{
	SearchAStar* aStarSearch = new SearchAStar;
	aStarSearch->FindBestPath(maze->start, maze->end);

	const std::list<const GraphNode*> path = aStarSearch->GetFinalPath();
	delete aStarSearch;

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

void Server::AvatarBegin()
{
	currentLink->move = true;
	currentLink->avatarPNode = new PhysicsNode();
	PhysicsEngine::Instance()->AddPhysicsObject(currentLink->avatarPNode);

	currentLink->avatarPos.x = currentLink->start.x;
	currentLink->avatarPos.y = currentLink->start.y;
	currentLink->avatarPNode->SetPosition(Vector3(currentLink->avatarPos.x, 0.0f, currentLink->avatarPos.y));

	SetAvatarVelocity();
}

void Server::StopAvatars()
{
	for (auto it = clients.begin(); it != clients.end(); ++it)
		(*it)->Init();
}

void Server::SetAvatarVelocity(ConnectedClient* client)
{
	if (!client)
		client = currentLink;

	Vector2 nodeA;
	nodeA.x = maze->allNodes[client->pathIndices[client->currentAvatarIndex]]._pos.x;
	nodeA.y = maze->allNodes[client->pathIndices[client->currentAvatarIndex]]._pos.y;
	Vector2 nodeB;
	nodeB.x = maze->allNodes[client->pathIndices[client->currentAvatarIndex + 1]]._pos.x;
	nodeB.y = maze->allNodes[client->pathIndices[client->currentAvatarIndex + 1]]._pos.y;
	Vector2 nodeToNode = nodeB - nodeA;
	float nodeToNodeDist = nodeToNode.Length();
	client->timeStep = nodeToNodeDist / avatarSpeed;
	client->timeToNext = client->timeStep;

	client->avatarVel = nodeToNode.Normalise() * (MAGIC_SCALAR * avatarSpeed * nodeToNodeDist);
	client->avatarPNode->SetLinearVelocity(Vector3(client->avatarVel.x, 0.0f, client->avatarVel.y));
}

void Server::TransmitAvatarPosition(ConnectedClient* client)
{
	AvatarPositionPacket* posPacket = new AvatarPositionPacket(client->avatarPos);
	packetHandler->SendPacket(client->address, posPacket);
	delete posPacket;
}

void Server::BroadcastAvatar()
{
	NewAvatarPacket* newAvatar = new NewAvatarPacket(currentLink->avatarPos, currentLink->colour, currentLink->clientID);
	packetHandler->BroadcastPacket(newAvatar);
	delete newAvatar;
}
