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

		uint drawFlags = PhysicsEngine::Instance()->GetDebugDrawFlags();
		drawFlags ^= DEBUGDRAW_FLAGS_CONSTRAINT;
		PhysicsEngine::Instance()->SetDebugDrawFlags(drawFlags);

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

		for (int i = 0; i < CLOTH_SIZE_X - 1; ++i)
		{
			for (int j = 0; j < CLOTH_SIZE_Y - 1; ++j)
			{
				int renderIndex = i * 2 * (CLOTH_SIZE_Y - 1) + 2 * j;
				int index = 1 + (i * CLOTH_SIZE_Y) + j;
				Vector3 triangle[3];
				triangle[0] = m_vpObjects[index]->Physics()->GetPosition();
				triangle[1] = m_vpObjects[index + 1]->Physics()->GetPosition();
				triangle[2] = m_vpObjects[index + CLOTH_SIZE_X]->Physics()->GetPosition();
				renderObjs[renderIndex] = new RenderNode(GenerateTriangle(triangle), Vector4(1.0f, 0.0f, 0.0f, 1.0f));
				GraphicsPipeline::Instance()->AddRenderNode(renderObjs[renderIndex]);

				triangle[0] = m_vpObjects[index + 1]->Physics()->GetPosition();
				triangle[1] = m_vpObjects[index + CLOTH_SIZE_X + 1]->Physics()->GetPosition();
				triangle[2] = m_vpObjects[index + CLOTH_SIZE_X]->Physics()->GetPosition();
				renderObjs[renderIndex + 1] = new RenderNode(GenerateTriangle(triangle), Vector4(1.0f, 0.0f, 0.0f, 1.0f));

				GraphicsPipeline::Instance()->AddRenderNode(renderObjs[renderIndex + 1]);
			}
		}
	}

	virtual void OnUpdateScene(float dt) override
	{
		Scene::OnUpdateScene(dt);

		BuildMesh();
	}

private:
	// two triangles per square, (CLOTH_SIZE_X - 1) * (CLOTH_SIZE_Y - 1) squares
	RenderNode* renderObjs[2 * (CLOTH_SIZE_X - 1) * (CLOTH_SIZE_Y - 1)];

	void BuildMesh()
	{
		for (int i = 0; i < CLOTH_SIZE_X - 1; ++i)
		{
			for (int j = 0; j < CLOTH_SIZE_Y - 1; ++j)
			{
				int renderIndex = i * 2 * (CLOTH_SIZE_Y - 1) + 2 * j;
				int index = 1 + (i * CLOTH_SIZE_Y) + j;
				Vector3 triangle[3];
				triangle[0] = m_vpObjects[index]->Physics()->GetPosition();
				triangle[1] = m_vpObjects[index + 1]->Physics()->GetPosition();
				triangle[2] = m_vpObjects[index + CLOTH_SIZE_X]->Physics()->GetPosition();
				UpdateTraingleMesh(renderObjs[renderIndex]->GetMesh(), triangle);

				triangle[0] = m_vpObjects[index + 1]->Physics()->GetPosition();
				triangle[1] = m_vpObjects[index + CLOTH_SIZE_X + 1]->Physics()->GetPosition();
				triangle[2] = m_vpObjects[index + CLOTH_SIZE_X]->Physics()->GetPosition();
				UpdateTraingleMesh(renderObjs[renderIndex + 1]->GetMesh(), triangle);
			}
		}
	}

	Mesh* GenerateTriangle(Vector3 *points)
	{
		Mesh*m = new Mesh();
		m->numVertices = 3;

		m->vertices = new Vector3[m->numVertices];
		m->vertices[0] = points[0];
		m->vertices[1] = points[1];
		m->vertices[2] = points[2];

		m->textureCoords = new Vector2[m->numVertices];
		m->textureCoords[0] = Vector2(points[0].x, points[0].y);
		m->textureCoords[1] = Vector2(points[1].x, points[1].y);
		m->textureCoords[2] = Vector2(points[2].x, points[2].y);

		m->colours = new Vector4[m->numVertices];
		m->colours[0] = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
		m->colours[1] = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
		m->colours[2] = Vector4(1.0f, 0.0f, 0.0f, 1.0f);

		m->GenerateNormals();
		m->GenerateTangents();
		m->BufferData();

		return m;
	}

	void UpdateTraingleMesh(Mesh* m, Vector3* points)
	{
		m->vertices[0] = points[0];
		m->vertices[1] = points[1];
		m->vertices[2] = points[2];

		m->textureCoords[0] = Vector2(points[0].x, points[0].y);
		m->textureCoords[1] = Vector2(points[1].x, points[1].y);
		m->textureCoords[2] = Vector2(points[2].x, points[2].y);

		m->GenerateNormals();
		m->GenerateTangents();
		m->BufferData();
	}
};