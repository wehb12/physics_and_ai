#pragma once

#include <ncltech\Scene.h>
#include <ncltech\NetworkBase.h>
#include <ncltech\SceneManager.h>
#include <ncltech\PhysicsEngine.h>
#include <nclgl\NCLDebug.h>
#include <ncltech\DistanceConstraint.h>
#include <ncltech\CommonUtils.h>
#include <string>
#include "NetworkEntity.h"

#include "AllPacketTypes.h"

//Basic Network Example

class Client : public Scene
{
public:
	Client(const std::string& friendly_name, NetworkEntity* thisEntity);

	virtual void OnInitializeScene() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdateScene(float dt) override;


	void ProcessNetworkEvent(const ENetEvent& evnt);

protected:
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

	NetworkEntity* packetHandler;

	int mazeSize;
	float mazeDensity;

	// bool that controls which position (start/ end) the user is moving
	bool start;
};