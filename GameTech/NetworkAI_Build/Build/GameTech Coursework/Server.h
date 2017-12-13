/******************************************************************************
Class: Server
Implements:
Author:
Will Hinds      <w.hinds2@newcastle.ac.uk>
Description:

Every entity connected to the network has an instantiation of the NetworkEntity
which handles packet transmissions and receipt

*//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <iostream>
#include <vector>
#include <ncltech\NetworkBase.h>
#include "../nclgl/Vector3.h"
#include "../nclgl/TSingleton.h"
#include "SearchAStar.h"
#include "MazeGenerator.h"

#include "AllPacketTypes.h"

#define TIME_STEP 1.0f / 30.0f
#define SPEED 2.0f

struct ConnectedClient
{
	ENetPeer* address;
	Vector3 start, end;
	SearchAStar* path;
	int* pathIndices;
	int pathLength;
	bool move;
	Vector2 avatarPos;
	Vector2 avatarVel;

	// this is the index into the pathIndices array
	// not the value in pathIndices array that the avatar
	// is positioned at
	int currentAvatarIndex;
	float timeToNext;
	float timeStep;

	PhysicsNode* avatarPNode;

	void Init()
	{
		address = NULL;
		start = Vector3(0, 0, 0);
		end = Vector3(0, 0, 0);
		path = new SearchAStar();
		pathIndices = NULL;
		pathLength = 0;
		move = false;
		avatarPos = Vector2(0, 0);
		avatarVel = Vector2(0, 0);
		currentAvatarIndex = 0;
		timeToNext = 0.0f;
		timeStep = 0.0f;
		
		avatarPNode = NULL;
	}
};

class PacketHandler;

class Server : public TSingleton<Server>
{
public:
	Server() :
		packetHandler(NULL),
		currentLink(NULL),
		mazeData(NULL),
		mazeParams(NULL),
		timeElapsed(0.0f),
		avatarSpeed(SPEED)
	{ }

	~Server()
	{
		int size = clients.size();
		for (int i = 0; i < size; ++i)
		{
			SAFE_DELETE(clients[i]->path);
			clients[i]->path = NULL;
			if (clients[i]->pathIndices)
				delete[] clients[i]->pathIndices;
			clients[i]->pathIndices = NULL;
			SAFE_DELETE(clients[i]);
		}
		clients.clear();

		SAFE_DELETE(maze);
		SAFE_DELETE(mazeData);
		SAFE_DELETE(mazeParams);
	}

	void Update(float msec);

	void CreateNewMaze(int size, float density);
	void PopulateEdgeList(bool* edgeList);
	void UpdateMazePositions(int indexStart, int indexEnd);
	void UpdateClientPath();
	void AvatarBegin();

//////// SETTERS ////////
	inline void SetPacketHandler(PacketHandler* pktHndl)	{ packetHandler = pktHndl; }
	inline void SetMaze(MazeGenerator* maze)				{ this->maze = maze; }
	void SetCurrentSender(ENetPeer* address);
	inline void SetMazeParamsPacket(Packet* dataPacket)		{ SAFE_DELETE(mazeParams); mazeParams = dataPacket; }
	inline void SetMazeDataPacket(Packet* dataPacket)		{ SAFE_DELETE(mazeData); mazeData = dataPacket; }

//////// GETTERS ////////
	inline ENetPeer* GetCurrentLinkAddress()				{ if (currentLink) return currentLink->address; }
	inline int* GetPathIndices()							{ return currentLink->pathIndices; }
	inline int GetPathLength()								{ return currentLink->pathLength; }
	inline int GetMazeSize()								{ return maze->size; }
	inline Packet* GetMazeParamsPacket()					{ return mazeParams; }
	inline Packet* GetMazeDataPacket()						{ return mazeData; }

//////// CLIENT HANDLING ////////
	ConnectedClient* CreateClient(ENetPeer* address);
	void AddClientPositons(Vector3 start, Vector3 end);
	inline void AddClientPath(SearchAStar* path) { currentLink->path = path; }
	void RemoveClient(ENetPeer* address);
private:
	ConnectedClient* GetClient(ENetPeer* address);
	inline void AddClient(ConnectedClient* client) { clients.push_back(client); }

	void UpdateAvatarPositions(float msec);
	void SetAvatarVelocity(ConnectedClient* client = NULL);
	void TransmitAvatarPosition(ConnectedClient* client);

private:
	MazeGenerator* maze;

	PacketHandler* packetHandler;

	ConnectedClient* currentLink;
	std::vector<ConnectedClient*> clients;

	// store this ready to send to a new client
	Packet* mazeData;
	Packet* mazeParams;

	float timeElapsed;
	//the time it takes an avatar to move from one grid square to thge next
	float avatarSpeed;
};