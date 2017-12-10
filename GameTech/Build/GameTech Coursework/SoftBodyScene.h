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
#include "../ncltech/DistanceConstraint.h"
#include "../ncltech/SphereCollisionShape.h"

#ifndef GRAVITY
#define GRAVITY 9.81f
#endif

#define CLOTH_SIZE_X 5
#define CLOTH_SIZE_Y 5

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
			true,
			0.0f,
			true,
			false,
			Vector4(0.2f, 0.5f, 1.0f, 1.0f)));

		float invMass = 0.01f;
		for (int i = 0; i < CLOTH_SIZE_X; ++i)
		{
			for (int j = 0; j < CLOTH_SIZE_Y; ++j)
			{
				PhysicsNode* pointPhys = new PhysicsNode();
				SphereCollisionShape* sphere = new SphereCollisionShape(0.45f);
				pointPhys->SetCollisionShape(sphere);
				pointPhys->SetPosition(Vector3(i - 2.5f, j + 5, 0));
				pointPhys->SetInverseMass(invMass);
				pointPhys->SetInverseInertia(sphere->BuildInverseInertia(invMass));
				pointPhys->SetElasticity(0.2f);

				//RenderNode* pointRen = new RenderNode();

				GameObject* pointObj = new GameObject("cloth_point" + to_string(i * CLOTH_SIZE_Y + j));

				pointObj->SetPhysics(pointPhys);
				pointObj->SetBoundingRadius(0.5f);

				this->AddGameObject(pointObj);
			}
		}
		m_vpObjects.back()->Physics()->SetInverseMass(0.0f);
		m_vpObjects[CLOTH_SIZE_Y]->Physics()->SetInverseMass(0.0f);


		for (int i = 0; i < CLOTH_SIZE_X; ++i)
		{
			for (int j = 0; j < CLOTH_SIZE_Y; ++j)
			{
				int index = 1 + i * CLOTH_SIZE_Y + j;
				PhysicsNode* thisNode = m_vpObjects[index]->Physics();
				Vector3 thisPos = thisNode->GetPosition();

				// adds a constraint between this node and the node at index 'nextIndex'
				auto AddConstraint = [&](int nextIndex)
				{
					//DistanceConstraint* constraint = new DistanceConstraint(thisNode, m_vpObjects[nextIndex]->Physics(),
					//	thisPos, m_vpObjects[nextIndex]->Physics()->GetPosition());
					SpringConstraint* constraint = new SpringConstraint(thisNode, m_vpObjects[nextIndex]->Physics(),
						thisPos, m_vpObjects[nextIndex]->Physics()->GetPosition(),
						2.0f, 1.0f);

					PhysicsEngine::Instance()->AddConstraint(constraint);
				};

				// need to account for edge cases where one of the CLOTH_SIZES is 1 (??)
				if (j == CLOTH_SIZE_Y - 1)
				{
					if (i != CLOTH_SIZE_X - 1)
					{
						// add a constraint to the next node in X
						AddConstraint(index + CLOTH_SIZE_Y);
					}
				}
				else
				{
					// add a constraint to the next node in Y
					AddConstraint(index + 1);

					if (i != CLOTH_SIZE_X - 1)
					{
						// add a constraint to the next node in X
						AddConstraint(index + CLOTH_SIZE_Y);
						// add a constraint to the node diagonally up and to the right
						AddConstraint(index + CLOTH_SIZE_Y + 1);
					}
					if (i != 0)
					{
						// add a constraint to the node diagonally up and to the left
						AddConstraint(index - CLOTH_SIZE_Y + 1);
					}
				}
			}
		}
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