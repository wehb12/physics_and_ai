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

class NetworkEntity
{
	friend class Client;
public:
	NetworkEntity(enet_uint8 type, ENetHost* host) :
		type(type),
		networkHost(host),
		serverConnection(NULL),
		mazeSize(0),
		mazeDensity(0.0f),
		maze(NULL),
		mazeRender(NULL),
		wallmesh(NULL),
		aStarSearch(new SearchAStar())
	{
	}

	~NetworkEntity()
	{
		CleanUp();
		SAFE_DELETE(maze);
		SAFE_DELETE(wallmesh);
		SAFE_DELETE(aStarSearch);
	}

	string HandlePacket(const ENetPacket* packet);

	void CleanUp()
	{
		mazeRender = NULL;	// should be deleted by Scene->RemoveAllGameObjects (??)
	}


//////// SEND PACKETS ////////
	void BroadcastPacket(Packet* packet);
	void SendPacket(ENetPeer* destination, Packet* packet);


//////// SETTERS ////////
	inline void SetServerConnection(ENetPeer* server) { serverConnection = server; }
	inline void SetCurrentSender(ENetPeer* sender) { currentPacketSender = sender; }

//////// GETTERS ////////
	inline enet_uint8 GetType()				{ return type; }
	inline int GetCurrentMazeSize()			{ return mazeSize; }
	inline float GetCurrentMazeDensity()	{ return mazeDensity; }
	inline MazeGenerator* GetMaze()			{ return maze; }

protected:
	inline bool GetPrintPathState()			{ return printPath; }

protected:
	int mazeSize;
	float mazeDensity;
	MazeGenerator* maze;
	MazeRenderer* mazeRender;
	SearchAStar* aStarSearch;
	Mesh* wallmesh;

	bool printPath;
	int* path;
	int pathLength;

private:
	void HandleMazeRequestPacket(MazeRequestPacket* reqPacket);

	template <class DataPacket>
	void HandleMazeDataPacket(DataPacket* dataPacket);
	void SendPositionPacket();

	template <class PositionPacket>
	void HandlePositionPacket(PositionPacket* posPacket);

	template <class PathPacket>
	void PopulatePath(PathPacket* pathPacket);

private:
	enet_uint8 type;

	ENetHost*	networkHost;
	ENetPeer*	serverConnection;
	ENetPeer*	currentPacketSender;
};