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

#include "PacketTypes.h"

enum packetFlags
{
	CREATE_MAZE = 0
};

//Basic Network Example

class Net1_Client : public Scene
{
public:
	Net1_Client(const std::string& friendly_name, NetworkEntity* thisEntity);

	virtual void OnInitializeScene() override;
	virtual void OnCleanupScene() override;
	virtual void OnUpdateScene(float dt) override;


	void ProcessNetworkEvent(const ENetEvent& evnt);

protected:
	void HandleKeyboardInput();

protected:
	GameObject* box;

	NetworkBase network;
	ENetPeer*	serverConnection;

	NetworkEntity* packetHandler;

	int mazeSize;
	float mazeDensity;
};