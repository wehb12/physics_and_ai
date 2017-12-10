// Author Will Hinds
// First attempt at GPU accelrated physics

#pragma once

#include <nclgl\NCLDebug.h>
#include <ncltech\Scene.h>
#include <ncltech\SceneManager.h>
#include <ncltech\CommonUtils.h>
#include <ncltech\PhysicsEngine.h>
#include <stdlib.h>
#include "../ncltech/SpringConstraint.h"

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
				int index = (i * NUM_TARGETS_Y) + j;
				GameObject* target = CommonUtils::BuildCuboidObject(
					"Target " + to_string(index),
					Vector3(x, y, z),
					Vector3(TARGET_SIZE, TARGET_SIZE, 0.2f),
					true,
					invMass,
					true,
					false,
					colour);
				target->Physics()->SetElasticity(0.5f); //No elasticity (Little cheaty)
				target->Physics()->SetFriction(1.0f);
				target->Physics()->SetForce(Vector3(0.0f, 2 * GRAVITY / invMass, 0.0f));

				SpringConstraint* spring = new SpringConstraint(target->Physics(), Vector3(x, y, z), Vector3(x, y, z), 500.0f, 100.0f);
				PhysicsEngine::Instance()->AddConstraint(spring);

				auto ChangeColour = [&](PhysicsNode* A, PhysicsNode* B)
				{
					RenderNode* target = *A->GetParent()->Render()->GetChildIteratorStart();

					for (i = 0; i < NUM_TARGETS_X * NUM_TARGETS_Y; ++i)
					{
						if (target == renders[i])
							break;
					}

					if (timer[i] < 4.5f)
					{
						timer[i] = 5.0f;

						if (target->GetColor().x < 0.1f)	// good target
						{
							target->SetColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f));

							int i = 0;

							score += 100;
						}
						else
							score -= 50;
					}

					return true;
				};

				target->Physics()->SetOnCollisionCallback(ChangeColour);
				renders[index] = *target->Render()->GetChildIteratorStart();
				timer[index] = 0.0f;

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

		for (int i = 0; i < NUM_TARGETS_X * NUM_TARGETS_Y; ++i)
		{
			if (timer[i] < 0.0f)
			{
				timer[i] = 0.0f;
				renders[i]->SetColor(Vector4(0.0f, 1.0f, 0.0f, 1.0f));
			}
			else if (timer[i] > 0.0f)
				timer[i] -= dt;
		}
	}

	inline void IncrementScore() { score += 100; }
	inline void DecrementScore() { score -= 50; }
private:
	int score;

	RenderNode* renders[NUM_TARGETS_X * NUM_TARGETS_Y];
	float timer[NUM_TARGETS_X * NUM_TARGETS_Y];
};