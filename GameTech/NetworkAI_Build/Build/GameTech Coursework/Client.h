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

//////// SETTERS ////////
	inline void SetPacketHandler(PacketHandler* pktHndl)	{ packetHandler = pktHndl; }
	inline void SetName(string name)						{ m_SceneName = name; }
	inline void SetPrintPathState(bool set = true)			{ printPath = set; }

//////// GETTERS ////////
	inline ENetPeer* GetServerConnection() { return serverConnection; }

protected:
	void GenerateEmptyMaze();
	void CreateAvatar();

	void HandleKeyboardInput();
	void PrintPath();
	void MoveNodeUp(bool start, int index);
	void MoveNodeDown(bool start, int index);
	void MoveNodeLeft(bool start, int index);
	void MoveNodeRight(bool start, int index);

	void ReconstructPosition(bool ifStart);

protected:
	GameObject* box;

	NetworkBase network;
	ENetPeer*	serverConnection;

	PacketHandler* packetHandler;

	int mazeSize;
	float mazeDensity;
	float lastDensity;

	int startIndex;
	int endIndex;
	Vector3 avatarPosition;

	MazeGenerator* maze;
	MazeRenderer* mazeRender;
	Mesh* wallmesh;

	int* path;
	int pathLength;
	bool printPath;

	// bool that controls which position (start/ end) the user is moving
	bool start;
};