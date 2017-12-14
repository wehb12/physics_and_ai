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

#define CLOTH_SIZE_X 10
#define CLOTH_SIZE_Y 5
#define SEPARATION 0.5f
#define MAX_X CLOTH_SIZE_X / 2
#define MIN_X - CLOTH_SIZE_X / 2
#define MIN_Y 5
#define MAX_Y MIN_Y + CLOTH_SIZE_Y

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
				int index = i * CLOTH_SIZE_Y + j;
				PhysicsNode* pointPhys = new PhysicsNode();
				SphereCollisionShape* sphere = new SphereCollisionShape(SEPARATION / 2);
				pointPhys->SetCollisionShape(sphere);
				pointPhys->SetPosition(Vector3(i * SEPARATION + MIN_X, j * SEPARATION + MIN_Y, 0));
				pointPhys->SetInverseMass(invMass);
				pointPhys->SetInverseInertia(sphere->BuildInverseInertia(invMass));
				pointPhys->SetElasticity(0.2f);

				GameObject* pointObj = new GameObject("cloth_point" + to_string(i * CLOTH_SIZE_Y + j));
				pointObj->SetPhysics(pointPhys);
				pointObj->SetBoundingRadius(SEPARATION / 2);

				this->AddGameObject(pointObj);
			}
		}

		// make top row of points non-collidable
		for (int i = 1; i <= m_vpObjects.size() / CLOTH_SIZE_Y; ++i)
			m_vpObjects[i * CLOTH_SIZE_Y]->Physics()->SetInverseMass(0.0f);

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
						1.0f, 1.0f);

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

		tex = SOIL_load_OGL_texture(
			TEXTUREDIR"Black_American_Flag.jpg",
			SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
			SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT);

		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);

		for (int i = 0; i < CLOTH_SIZE_X - 1; ++i)
		{
			for (int j = 0; j < CLOTH_SIZE_Y - 1; ++j)
			{
				int renderIndex = i * 2 * (CLOTH_SIZE_Y - 1) + 2 * j;
				int index = 1 + (i * CLOTH_SIZE_Y) + j;
				Vector3 triangle[3];
				triangle[0] = m_vpObjects[index]->Physics()->GetPosition();
				triangle[1] = m_vpObjects[index + 1]->Physics()->GetPosition();
				triangle[2] = m_vpObjects[index + CLOTH_SIZE_Y]->Physics()->GetPosition();
				if (renderObjs[renderIndex])
				{
					Mesh* m = renderObjs[renderIndex]->GetMesh();
					SAFE_DELETE(m);
					SAFE_DELETE(renderObjs[renderIndex]);
				}
				renderObjs[renderIndex] = new RenderNode(GenerateTriangle(triangle), Vector4(1.0f, 1.0f, 1.0f, 1.0f));
				GraphicsPipeline::Instance()->AddRenderNode(renderObjs[renderIndex]);

				triangle[0] = m_vpObjects[index + 1]->Physics()->GetPosition();
				triangle[1] = m_vpObjects[index + CLOTH_SIZE_Y + 1]->Physics()->GetPosition();
				triangle[2] = m_vpObjects[index + CLOTH_SIZE_Y]->Physics()->GetPosition();
				if (renderObjs[renderIndex + 1])
				{
					Mesh* m = renderObjs[renderIndex + 1]->GetMesh();
					SAFE_DELETE(m);
					SAFE_DELETE(renderObjs[renderIndex + 1]);
				}
				renderObjs[renderIndex + 1] = new RenderNode(GenerateTriangle(triangle), Vector4(1.0f, 1.0f, 1.0f, 1.0f));

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
	GLuint tex;

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
				triangle[2] = m_vpObjects[index + CLOTH_SIZE_Y]->Physics()->GetPosition();
				UpdateTraingleMesh(renderObjs[renderIndex]->GetMesh(), triangle);

				triangle[0] = m_vpObjects[index + 1]->Physics()->GetPosition();
				triangle[1] = m_vpObjects[index + CLOTH_SIZE_Y + 1]->Physics()->GetPosition();
				triangle[2] = m_vpObjects[index + CLOTH_SIZE_Y]->Physics()->GetPosition();
				UpdateTraingleMesh(renderObjs[renderIndex + 1]->GetMesh(), triangle);
			}
		}
	}

	Mesh* GenerateTriangle(Vector3 *points)
	{
		Mesh* m = new Mesh();
		m->numVertices = 3;

		m->vertices = new Vector3[m->numVertices];
		m->vertices[0] = points[0];
		m->vertices[1] = points[1];
		m->vertices[2] = points[2];

		m->textureCoords = new Vector2[m->numVertices];
		m->textureCoords[0] = Vector2(points[0].x / (-(MAX_X - MIN_X) * SEPARATION), points[0].y / ((MAX_X - MIN_X) * SEPARATION* SEPARATION));
		m->textureCoords[1] = Vector2(points[1].x / (-(MAX_X - MIN_X) * SEPARATION), points[1].y / ((MAX_X - MIN_X) * SEPARATION* SEPARATION));
		m->textureCoords[2] = Vector2(points[2].x / (-(MAX_X - MIN_X) * SEPARATION), points[2].y / ((MAX_X - MIN_X) * SEPARATION* SEPARATION));

		m->colours = new Vector4[m->numVertices];
		m->colours[0] = Vector4(1.0f, 1.0f, 1.0f, 0.0f);
		m->colours[1] = Vector4(1.0f, 1.0f, 1.0f, 0.0f);
		m->colours[2] = Vector4(1.0f, 1.0f, 1.0f, 0.0f);

		m->SetTexture(tex);
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

		m->GenerateNormals();
		m->GenerateTangents();
		UnBufferData(m);
		m->BufferData();
	}

	void UnBufferData(Mesh* m)
	{
		glBindVertexArray(m->arrayObject);

		glDeleteBuffers(1, &m->bufferObject[VERTEX_BUFFER]);

		if (m->textureCoords)
			glDeleteBuffers(1, &m->bufferObject[TEXTURE_BUFFER]);

		if (m->colours)
			glDeleteBuffers(1, &m->bufferObject[COLOUR_BUFFER]);

		if (m->normals)
			glDeleteBuffers(1, &m->bufferObject[NORMAL_BUFFER]);

		if (m->tangents)
			glDeleteBuffers(1, &m->bufferObject[TANGENT_BUFFER]);

		if (m->indices)
			glDeleteBuffers(1, &m->bufferObject[INDEX_BUFFER]);

		glBindVertexArray(0);
	}
};