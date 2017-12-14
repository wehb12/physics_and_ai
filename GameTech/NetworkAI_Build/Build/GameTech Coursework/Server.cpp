#include "PacketHandler.h"
#include "Server.h"

void Server::Update(float sec)
{
	timeElapsed += sec;

	if (timeElapsed > TIME_STEP)
	{
		if (clients.size() > 0)
			UpdateAvatarPositions(sec);

		timeElapsed = 0.0f;
	}
}

void Server::UpdateAvatarPositions(float sec)
{
	for (auto it = clients.begin(); it != clients.end(); ++it)
	{
		if ((*it)->move)
		{
			if ((*it)->timeToNext > 0)
			{
				// update velocity every go to combat physics engine damping
				(*it)->avatarPNode->SetLinearVelocity(Vector3((*it)->avatarVel.x, 0.0f, (*it)->avatarVel.y));

				(*it)->timeToNext -= timeElapsed;
				if (!(*it)->usePhysics)
					(*it)->avatarPos = (*it)->avatarPos + (*it)->avatarVel * timeElapsed;
				else
				{
					(*it)->avatarPos.x = (*it)->avatarPNode->GetPosition().x;
					(*it)->avatarPos.y = (*it)->avatarPNode->GetPosition().z;
				}
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
			BroadcastAvatarPosition(*it);
		}
	}
}

ConnectedClient* Server::GetClient(ENetPeer* address)
{
	for (auto it = clients.begin(); it != clients.end(); ++it)
	{
		if (AddressesEqual((*it)->peer, address))
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

	newClient->address = address->address;
	newClient->peer = address;

	AddClient(newClient);

	ConfirmConnectionPacket* packet = new ConfirmConnectionPacket(newClient->clientID);
	packetHandler->SendPacket(newClient->peer, packet);
	delete packet;

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
			if (AddressesEqual(currentLink->peer, address))
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
	currentPeer = address;
	ConnectedClient* clientFound = GetClient(address);
	if (!clientFound)
		clientFound = CreateClient(address);

	currentLink = clientFound;
}

void Server::UpdateMazePositions(int indexStart, int indexEnd)
{
	maze->start = &maze->allNodes[indexStart];
	maze->end = &maze->allNodes[indexEnd];

	currentLink->update = true;
	// if the end position has NOT been changed, don't update the path
	if (currentLink->pathLength > 0)
		currentLink->update = indexEnd != currentLink->pathIndices[currentLink->pathLength - 1];

	if (currentLink->update)
	{
		//currentLink->ChangeStart(indexStart);
		AddClientPositons(maze->start->_pos, maze->end->_pos);
		UpdateClientPath();
	}
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

	if (!currentLink->avatarPNode)
	{
		currentLink->avatarPNode = new PhysicsNode();
		PhysicsEngine::Instance()->AddPhysicsObject(currentLink->avatarPNode);
	}

	if (currentLink->update)
	{
		currentLink->avatarPos.x = currentLink->start.x;
		currentLink->avatarPos.y = currentLink->start.y;
		currentLink->avatarPNode->SetPosition(Vector3(currentLink->avatarPos.x, 0.0f, currentLink->avatarPos.y));

		SetAvatarVelocity();
	}
}

void Server::StopAvatars()
{
	for (auto it = clients.begin(); it != clients.end(); ++it)
		(*it)->Clear();
}

void Server::StopAvatar()
{
	currentLink->Clear();
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

	//client->avatarVel = nodeToNode.Normalise() * (MAGIC_SCALAR * avatarSpeed * nodeToNodeDist);
	client->avatarVel = nodeToNode * (avatarSpeed / nodeToNodeDist);
	client->avatarPNode->SetLinearVelocity(Vector3(client->avatarVel.x, 0.0f, client->avatarVel.y));
}

void Server::BroadcastAvatarPosition(ConnectedClient* client)
{
	AvatarPositionPacket* posPacket = new AvatarPositionPacket(client->avatarPos, client->currentAvatarIndex, client->clientID);
	packetHandler->BroadcastPacket(posPacket);
	delete posPacket;
}

void Server::BroadcastAvatar()
{
	NewAvatarPacket* newAvatar = new NewAvatarPacket(currentLink->avatarPos, currentLink->colour, currentLink->clientID);
	packetHandler->BroadcastPacket(newAvatar);
	delete newAvatar;
}
