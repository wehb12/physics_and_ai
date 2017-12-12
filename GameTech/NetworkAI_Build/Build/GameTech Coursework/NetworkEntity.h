#pragma once

#include <ncltech\NetworkBase.h>
#include <string>
#include "PacketTypes.h"
#include "MazeGenerator.h"
#include "MazeRenderer.h"
#include "../ncltech/SceneManager.h"

enum NetworkEntityType
{
	CLIENT,
	SERVER
};

class NetworkEntity
{
public:
	NetworkEntity(enet_uint8 type, ENetHost* host) :
		type(type),
		networkHost(host),
		mazeSize(0),
		maze(NULL),
		mazeRender(NULL)
	{ }

	~NetworkEntity()
	{
		SAFE_DELETE(maze);

		if (mazeRender)
		{
			SceneManager::Instance()->GetCurrentScene()->RemoveGameObject(mazeRender);
			delete mazeRender;
			mazeRender = NULL;
		}
	}

	string HandlePacket(const ENetPacket* packet);

	void CleanUp()
	{
		mazeRender = NULL;
		SAFE_DELETE(maze);
	}


//////// SEND PACKETS ////////
	void BroadcastPacket(Packet* packet);
	void SendPacket(ENetPeer* destination, Packet* packet);


//////// GETTERS ////////
	inline enet_uint8 GetType() { return type; }

private:
	enet_uint8 type;

	ENetHost* networkHost;

	int mazeSize;
	MazeGenerator* maze;
	MazeRenderer* mazeRender;
};