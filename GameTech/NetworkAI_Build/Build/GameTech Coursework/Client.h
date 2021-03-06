#pragma once

#include <ncltech\Scene.h>
#include <ncltech\NetworkBase.h>
#include <ncltech\SceneManager.h>
#include <ncltech\PhysicsEngine.h>
#include <nclgl\NCLDebug.h>
#include <ncltech\DistanceConstraint.h>
#include <ncltech\CommonUtils.h>
#include <string>
#include "../nclgl/TSingleton.h"
#include "MazeGenerator.h"
#include "MazeRenderer.h"
#include "../nclgl/OBJMesh.h"

#include "AllPacketTypes.h"

//Basic Network Example

class PacketHandler;

class Client : public Scene, public TSingleton<Client>
{
public:
	Client();
	~Client()
	{
		mazeRender = NULL;	// GameObject, should be deleted by Scene destructor
		SAFE_DELETE(maze);
		SAFE_DELETE(wallmesh);
	}

	virtual void OnInitializeScene() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdateScene(float dt) override;

	void ProcessNetworkEvent(const ENetEvent& evnt);

	void GenerateWalledMaze(bool* walledEdges, int numEdges);
	void RenderNewMaze();
	void PopulatePath(Packet* pathPacket);
	inline void CreateAvatar(float colour)
	{
		avatarColour.push_back(colour);
		if(maze)
			AddAvatar(colour);
	}
	inline void RemoveAvatar(int index)
	{
		mazeRender->RemoveAvatar(index);
		avatarPosition.erase(avatarPosition.begin() + index);
	}

//////// SETTERS ////////
	inline void SetPacketHandler(PacketHandler* pktHndl)	{ packetHandler = pktHndl; }
	inline void SetName(string name)						{ m_SceneName = name; }
	inline void SetPrintPathState(bool set = true)			{ printPath = set; }
	inline void SetMazeParameters(int size, float density)	{ mazeSize = size; mazeDensity = density; }
	inline void SetAvatarPosition(Vector2 pos, int iD)
	{
		if (path)
		{
			avatarPosition[iD] = pos;
			UpdateAvatar(iD);
		}
	}
	inline void SetID(enet_uint8 iD)						{ clientID = iD; }
	inline void SetCurrentAvatarIndex(enet_uint16 index)	{ currentIndex = index; }

//////// GETTERS ////////
	inline ENetPeer* GetServerConnection()	{ return serverConnection; }
	inline bool HasMaze()					{ return maze; }
	inline enet_uint8 GetID()				{ return clientID; }

protected:
	void GenerateEmptyMaze();

	void HandleKeyboardInput();
	void PrintPath();
	void MoveNodeUp(bool start, int index);
	void MoveNodeDown(bool start, int index);
	void MoveNodeLeft(bool start, int index);
	void MoveNodeRight(bool start, int index);

	void UpdatePosition(bool ifStart);
	void UpdateAvatar(int iD);
	void AddAvatar(float colour);

protected:
	enet_uint8 clientID;

	NetworkBase network;
	ENetPeer*	serverConnection;

	PacketHandler* packetHandler;

	int mazeSize;
	float mazeDensity;
	float lastDensity;

	int startIndex;
	int endIndex;
	int currentIndex;
	vector<Vector2> avatarPosition;
	vector<float> avatarColour;
	// bool that controls whether client avatar position is updated by physics engine or Server
	bool usePhysics;

	MazeGenerator* maze;
	// this tells you if the start node has been moved or not
	bool ifMoved;
	GraphNode startNode;
	GraphNode endNode;
	MazeRenderer* mazeRender;
	Mesh* wallmesh;

	int* path;
	int pathLength;
	bool printPath;
	bool stringPulling;

	// bool that controls which position (start/ end) the user is moving
	bool start;
};