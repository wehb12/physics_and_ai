/******************************************************************************
Class: NetworkEntity
Implements:
Author:
Will Hinds      <w.hinds2@newcastle.ac.uk>
Description:

Every entity connected to the network has an instantiation of the NetworkEntity
which handles packet transmissions and receipt

*//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ncltech\NetworkBase.h>
#include <string>
#include "AllPacketTypes.h"
#include "MazeGenerator.h"
#include "MazeRenderer.h"
#include "SearchAStar.h"
#include "../ncltech/SceneManager.h"
#include "../nclgl/OBJMesh.h"
#include "../ncltech/CommonUtils.h"

enum NetworkEntityType
{
	CLIENT,
	SERVER
};

class PacketHandler
{
	friend class Client;
	friend class Server;
public:
	PacketHandler(enet_uint8 type, ENetHost* host) :
		entityType(type),
		networkHost(host),
		lastInstr(0)
	{ }

	~PacketHandler()
	{

	}

	string HandlePacket(const ENetPacket* packet);

//////// SEND PACKETS ////////
	void BroadcastPacket(Packet* packet);
	void SendPacket(ENetPeer* destination, Packet* packet);

//////// GETTERS ////////
	inline enet_uint8 GetType()				 { return entityType; }

private:
	void HandleMazeRequestPacket(MazeParamsPacket* reqPacket);

	template <class DataPacket>
	void HandleMazeDataPacket(DataPacket* dataPacket);
	void SendPositionPacket(ENetPeer* dest, MazeGenerator* maze);

	template <class PositionPacket>
	void HandlePositionPacket(PositionPacket* posPacket);

private:
	enet_uint8 entityType;
	ENetHost*	networkHost;

	enet_uint8 lastInstr;

	//int mazeSize;
	//float mazeDensity;
};