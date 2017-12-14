#pragma once

#include "../ncltech/GameObject.h"

class SoftBodySegment : public GameObject
{
public:
	SoftBodySegment(string name, RenderNode* rnode, PhysicsNode* pnode) :
	GameObject(name, rnode, pnode)
	{ }

	~SoftBodySegment()
	{
		for (int i = 0; i < rNodes.size(); ++i)
			GraphicsPipeline::Instance()->RemoveRenderNode(rNodes[i]);
	}

	void AddRenderSegment(RenderNode* rnode)
	{
		rNodes.push_back(rnode);
		GraphicsPipeline::Instance()->AddRenderNode(rnode);
	}

	void Update()
	{
		for (int i = 0; i < rNodes.size(); ++i)
			rNodes[i]->SetTransform(physicsNode->GetWorldSpaceTransform());
	}

private:
	vector<RenderNode*> rNodes;
};