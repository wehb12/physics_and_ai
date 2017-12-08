// Author Will Hinds
// First attempt at GPU accelrated physics

#pragma once

#include <nclgl\NCLDebug.h>
#include <ncltech\Scene.h>
#include <ncltech\SceneManager.h>
#include <ncltech\CommonUtils.h>
#include <ncltech\PhysicsEngine.h>
#include <stdlib.h>

#define X_DIMS 5
#define Y_DIMS 5
#define Z_DIMS 0
#define NUM_TARGETS_X 3
#define NUM_TARGETS_Y 3

#define TARGET_SIZE 1.0f
#ifndef GRAVITY
#define GRAVITY 9.81f
#endif

class TargetPractise : public Scene
{
public:
	TargetPractise(const std::string& friendly_name)
		: Scene(friendly_name),
		score(0)
	{ }

	virtual void OnInitializeScene() override
	{
		float invMass = 0.1f;
		//create some targets
		for (int i = 0; i < NUM_TARGETS_X; ++i)
		{
			for (int j = 0; j < NUM_TARGETS_X; ++j)
			{
				Vector4 colour = Vector4(0.0f, 1.0f, 0.0f, 1.0f);
				float x = i * X_DIMS;
				float y = j * Y_DIMS;
				float z = ((float)(rand() % 101) / 100.0f) * 4;
				GameObject* target = CommonUtils::BuildCuboidObject(
					"Target " + to_string((i * NUM_TARGETS_Y) + j),
					Vector3(x, y, z),
					Vector3(TARGET_SIZE, TARGET_SIZE, TARGET_SIZE),
					true,
					invMass,
					true,
					false,
					colour);
				target->Physics()->SetElasticity(0.5f); //No elasticity (Little cheaty)
				target->Physics()->SetFriction(1.0f);
				target->Physics()->SetForce(Vector3(0.0f, 2 * GRAVITY / invMass, 0.0f));

				auto ChangeColour = [&](PhysicsNode* A, PhysicsNode* B)
				{
					if (A->GetParent()->GetName().compare(0, 6, "Target") == 0)
						A->GetParent()->Render()->SetColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f));
					else if (B->GetParent()->GetName().compare(0, 6, "Target") == 0)
						B->GetParent()->Render()->SetColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f));

					return true;
				};

				PhysicsCollisionCallback IfHit = ChangeColour;

				target->Physics()->SetOnCollisionCallback(ChangeColour);

				this->AddGameObject(target);
			}
		}
	}

	virtual void OnUpdateScene(float dt) override
	{
		Scene::OnUpdateScene(dt);

		uint drawFlags = PhysicsEngine::Instance()->GetDebugDrawFlags();

		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_U))
			PhysicsEngine::Instance()->ToggleGPUAcceleration();

		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "    Score: %d", score);
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "    Press [J] to fire a SPHERE");
		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "    Press [K] to fire a CUBE");
	}

	inline void IncrementScore() { score += 100; }
	inline void DecrementScore() { score -= 50; }
private:
	int score;
};