#pragma once

#include <ncltech\GameObject.h>
#include <ncltech\CommonMeshes.h>
#include "MazeGenerator.h"
#include "SearchAlgorithm.h"


struct WallDescriptor
{
	uint _xs, _xe;
	uint _ys, _ye;

	WallDescriptor(uint x, uint y) : _xs(x), _xe(x + 1), _ys(y), _ye(y + 1) {}
};

typedef std::vector<WallDescriptor> WallDescriptorVec;


class MazeRenderer : public GameObject
{
public:
	MazeRenderer(MazeGenerator* gen, Mesh* wallmesh = CommonMeshes::Cube());
	virtual ~MazeRenderer();

	//The search history draws from edges because they already store the 'to'
	// and 'from' of GraphNodes.
	void DrawSearchHistory(const SearchHistory& history, float line_width);

	void UpdateStartNodeTransform(Matrix4 transform)
	{
		(*(Render()->GetChildIteratorEnd() - 2 - numAvatars))->SetTransform(transform);
	}

	void UpdateEndNodeTransform(Matrix4 transform)
	{
		(*(Render()->GetChildIteratorEnd() - 1 - numAvatars))->SetTransform(transform);
	}

	void UpdateAvatarTransform(Matrix4 transform , int index = 0)
	{
		if (index < numAvatars)
			(*(Render()->GetChildIteratorEnd() - numAvatars + index))->SetTransform(transform);
		else
			NCLERROR("There are not that many avatars");
	}

	inline void AddAvatar(RenderNode* avatar)		{ ++numAvatars; Render()->AddChild(avatar); }
	inline void RemoveAvatar(int index)
	{
		RenderNode* toRemove = NULL;
		if (index < numAvatars)
			toRemove = *(Render()->GetChildIteratorEnd() - numAvatars + index);
		renderNode->RemoveChild(toRemove);
		SAFE_DELETE(toRemove);
		--numAvatars;
	}


protected:
	//Turn MazeGenerator data into flat 2D map (3 size x 3 size) of boolean's
	// - True for wall
	// - False for empty
	//Returns uint: Guess at the number of walls required
	uint Generate_FlatMaze();

	//Construct a list of WallDescriptors from the flat 2D map generated above.
	void Generate_ConstructWalls();

	//Finally turns the wall descriptors into actual renderable walls ready
	// to be seen on screen.
	void Generate_BuildRenderNodes();

protected:
	MazeGenerator*	maze;
	Mesh*			mesh;

	bool*	flat_maze;	//[flat_maze_size x flat_maze_size]
	uint	flat_maze_size;

	WallDescriptorVec	wall_descriptors;

	int numAvatars;
};