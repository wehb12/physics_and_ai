// Author Will Hinds
// Non-tearable soft body

#pragma once

#include <nclgl\NCLDebug.h>
#include <ncltech\Scene.h>
#include <ncltech\SceneManager.h>
#include <ncltech\CommonUtils.h>
#include <ncltech\PhysicsEngine.h>
#include <stdlib.h>
#include "../ncltech/SpringConstraint.h"

#ifndef GRAVITY
#define GRAVITY 9.81f
#endif

class SoftBodyScene : public Scene
{
public:
	SoftBodyScene(const std::string& friendly_name)
		: Scene(friendly_name)
	{ }

	virtual void OnInitializeScene() override
	{
		Scene::OnInitializeScene();

		this->AddGameObject(CommonUtils::BuildCuboidObject(
			"Ground",
			Vector3(0.0f, -1.5f, 0.0f),
			Vector3(20.0f, 1.0f, 20.0f),
			false,
			0.0f,
			false,
			false,
			Vector4(0.2f, 0.5f, 1.0f, 1.0f)));
	}

	virtual void OnUpdateScene(float dt) override
	{
		Scene::OnUpdateScene(dt);

		uint drawFlags = PhysicsEngine::Instance()->GetDebugDrawFlags();

		//NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "Score: %d", score);
		//NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "");
		//NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "--- Controls ---");
		//NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "    Press [J] to fire a SPHERE");
		//NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "    Press [K] to fire a CUBE");

	
	}
};