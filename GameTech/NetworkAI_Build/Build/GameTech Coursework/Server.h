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

struct ConnectedClient
{
	ENetPeer* address;
	Vector3 start, end;
	SearchAStar* path;
	int* pathIndices;
	int pathLength;

	void Init()
	{
		address = NULL;
		start = Vector3(0, 0, 0);
		end = Vector3(0, 0, 0);
		path = new SearchAStar();
		pathIndices = NULL;
		pathLength = 0;
	}
};

class PacketHandler;

class Server : public TSingleton<Server>
{
public:
	Server() :
		packetHandler(NULL),
		currentLink(NULL),
		mazeData(NULL)
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
	}

	void CreateNewMaze(int size, float density);
	void PopulateEdgeList(bool* edgeList);
	void UpdateMazePositions(int indexStart, int indexEnd);
	void UpdateClientPath();

//////// SETTERS ////////
	inline void SetPacketHandler(PacketHandler* pktHndl)	{ packetHandler = pktHndl; }
	inline void SetMaze(MazeGenerator* maze)				{ this->maze = maze; }
	void SetCurrentSender(ENetPeer* address);
	inline void SetMazeDataPacket(Packet* dataPacket)		{ SAFE_DELETE(mazeData); mazeData = dataPacket; }

//////// GETTERS ////////
	inline ENetPeer* GetCurrentLinkAddress()				{ if (currentLink) return currentLink->address; }
	inline int* GetPathIndices()							{ return currentLink->pathIndices; }
	inline int GetPathLength()								{ return currentLink->pathLength; }
	inline int GetMazeSize()								{ return maze->size; }
	inline Packet* GetMazeDataPacket()						{ return mazeData; }

//////// CLIENT HANDLING ////////
	ConnectedClient* CreateClient(ENetPeer* address);
	void AddClientPositons(ENetPeer* address, Vector3 start, Vector3 end);
	void AddClientPath(ENetPeer* address, SearchAStar* path);
	void RemoveClient(ENetPeer* address);
private:
	ConnectedClient* GetClient(ENetPeer* address);
	inline void AddClient(ConnectedClient* client) { clients.push_back(client); }


private:
	MazeGenerator* maze;

	PacketHandler* packetHandler;

	ConnectedClient* currentLink;
	std::vector<ConnectedClient*> clients;

	// store this ready to send to a new client
	Packet* mazeData;
};