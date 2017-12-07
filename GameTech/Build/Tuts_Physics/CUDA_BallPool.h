// Author Will Hinds
// First attempt at GPU accelrated physics

#pragma once

#include <nclgl\NCLDebug.h>
#include <ncltech\Scene.h>
#include <ncltech\SceneManager.h>
#include <ncltech\CommonUtils.h>
#include <ncltech\PhysicsEngine.h>
#include <stdlib.h>

#define POOL_X 16
#define POOL_Y 2
#define POOL_Z 16

class CUDA_BallPool : public Scene
{
public:
	CUDA_BallPool(const std::string& friendly_name)
		: Scene(friendly_name)
	{}

	virtual void OnInitializeScene() override
	{
		//Create Ground
		this->AddGameObject(CommonUtils::BuildCuboidObject(
			"Ground",
			Vector3(0.0f, -1.0f, 0.0f),
			Vector3(POOL_X, 1.0f, POOL_Z),
			true,
			0.0f,
			true,
			false,
			Vector4(0.2f, 0.5f, 1.0f, 1.0f)));

		//Create see through walls
		this->AddGameObject(CommonUtils::BuildCuboidObject(
			"Wall",
			Vector3(0.0f, POOL_Y, POOL_Z + 1),
			Vector3(POOL_X, POOL_Y, 1.0f),
			true,
			0.0f,
			true,
			false,
			Vector4(0.2f, 0.5f, 1.0f, 0.1f)));
		this->AddGameObject(CommonUtils::BuildCuboidObject(
			"Wall",
			Vector3(0.0f, POOL_Y, -POOL_Z - 1),
			Vector3(POOL_X, POOL_Y, 1.0f),
			true,
			0.0f,
			true,
			false,
			Vector4(0.2f, 0.5f, 1.0f, 0.1f)));
		this->AddGameObject(CommonUtils::BuildCuboidObject(
			"Wall",
			Vector3(POOL_X + 1, POOL_Y, 0.0f),
			Vector3(1.0f, POOL_Y, POOL_Z),
			true,
			0.0f,
			true,
			false,
			Vector4(0.2f, 0.5f, 1.0f, 0.1f)));
		this->AddGameObject(CommonUtils::BuildCuboidObject(
			"Wall",
			Vector3(-POOL_X - 1, POOL_Y, 0.0f),
			Vector3(1.0f, POOL_Y, POOL_Z),
			true,
			0.0f,
			true,
			false,
			Vector4(0.2f, 0.5f, 1.0f, 0.1f)));

		//create some balls
		for (int i = 0; i < 1000; ++i)
		{
			Vector4 color = Vector4((float)(rand() % 101) / (float)(100), (float)(rand() % 101) / (float)(100), (float)(rand() % 101) / (float)(100), 1.0f);
			float x = (float)(rand() % 101) / (float)(100) * ((POOL_X - 4) * 2) - (POOL_X - 4);
			float y = (float)(rand() % 101) / (float)(100) * POOL_Y + 2;
			float z = (float)(rand() % 101) / (float)(100) * ((POOL_Z - 4) * 2) - (POOL_Z - 4);
			GameObject* ball = CommonUtils::BuildSphereObject(
				"ball",
				Vector3(x, y, z),
				0.5f,
				true,
				0.1f,
				true,
				true,
				color);
			ball->Physics()->SetElasticity(0.25f); //No elasticity (Little cheaty)
			ball->Physics()->SetFriction(1.0f);

			this->AddGameObject(ball);
		}
	}

	virtual void OnUpdateScene(float dt) override
	{
		Scene::OnUpdateScene(dt);

		uint drawFlags = PhysicsEngine::Instance()->GetDebugDrawFlags();

		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_U))
			PhysicsEngine::Instance()->ToggleGPUAcceleration();

		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "--- Controls ---");
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "    GPU Acceleration : %s ([U] to toggle)",
			PhysicsEngine::Instance()->GetGPUAccelerationState() ? "Enabled" : "Disabled");
	}
};