#pragma once

#include <ncltech\NetworkBase.h>
#include <string>
#include "PacketTypes.h"
#include "MazeGenerator.h"
#include "MazeRenderer.h"
#include "../ncltech/SceneManager.h"
#include "../nclgl/OBJMesh.h"

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
		mazeRender(NULL),
		wallmesh(NULL)
	{

	}

	~NetworkEntity()
	{
		SAFE_DELETE(maze);

		if (mazeRender)
		{
			SceneManager::Instance()->GetCurrentScene()->RemoveGameObject(mazeRender);
			delete mazeRender;
			mazeRender = NULL;
		}

		SAFE_DELETE(wallmesh);
	}

	string HandlePacket(const ENetPacket* packet);

	void CleanUp()
	{
		SAFE_DELETE(mazeRender);
		mazeRender = NULL;
		SAFE_DELETE(maze);
		maze = NULL;
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
	Mesh* wallmesh;
};