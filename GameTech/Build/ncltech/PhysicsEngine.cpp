#include "PhysicsEngine.h"
#include "GameObject.h"
#include "CollisionDetectionSAT.h"
#include <nclgl\NCLDebug.h>
#include <nclgl\Window.h>
#include <omp.h>
#include <algorithm>

extern "C" int CUDA_run(Vector3* cu_pos, float* cu_radius,
	Vector3* cu_globalOnA, Vector3* cu_globalOnB,
	Vector3* cu_normal, float* cu_penetration, int* cuda_nodeAIndex,
	int* cuda_nodeBIndex, int entities);
extern "C" bool CUDA_init(int arrSize);
extern "C" bool CUDA_free();

void PhysicsEngine::SetDefaults()
{
	//Variables set here /will/ be reset with each scene
	updateTimestep = 1.0f / 60.f;
	updateRealTimeAccum = 0.0f;
	gravity = Vector3(0.0f, -9.81f, 0.0f);
	dampingFactor = 0.999f;
}

PhysicsEngine::PhysicsEngine()
{
	//Variables set here will /not/ be reset with each scene
	isPaused = false;

	useOctree = false;
	ResetRoot();

	sphereSphere = false;

	gpuAccel = false;

	debugDrawFlags = 0;

	SetDefaults();
}

PhysicsEngine::~PhysicsEngine()
{
	if (gpuAccel)
		CUDA_free();

	RemoveAllPhysicsObjects();
	TerminateOctree(root);
}

void PhysicsEngine::AddPhysicsObject(PhysicsNode* obj)
{
	physicsNodes.push_back(obj);

	if (InOctree(root, obj))
		AddToOctree(root, obj);

	if (gpuAccel)
		CUDA_init(physicsNodes.size());
}

void PhysicsEngine::RemovePhysicsObject(PhysicsNode* obj)
{
	//Lookup the object in question
	auto found_loc = std::find(physicsNodes.begin(), physicsNodes.end(), obj);

	//If found, remove it from the list
	if (found_loc != physicsNodes.end())
	{
		physicsNodes.erase(found_loc);
		Octree* tree = obj->GetOctree();
		if (tree)
		{
			FindAndDelete(tree, obj);
		}
	}
}

void PhysicsEngine::RemoveAllPhysicsObjects()
{
	//Delete and remove all constraints/collision manifolds
	for (Constraint* c : constraints)
	{
		delete c;
		c = NULL;
	}
	constraints.clear();

	for (Manifold* m : manifolds)
	{
		delete m;
		m = NULL;
	}
	manifolds.clear();

	TerminateOctree(root);
	ResetRoot();

	//Delete and remove all physics objects
	// - we also need to inform the (possibly) associated game-object
	//   that the physics object no longer exists
	for (PhysicsNode* obj : physicsNodes)
	{
		if (obj->GetParent()) obj->GetParent()->SetPhysics(NULL);
		delete obj;
		obj = NULL;
	}
	physicsNodes.clear();
}

void PhysicsEngine::Update(float deltaTime)
{
	//The physics engine should run independantly to the renderer
	// - As our codebase is currently single threaded we just need
	//   a way of calling "UpdatePhysics()" at regular intervals
	//   or multiple times a frame if the physics timestep is higher
	//   than the renderers.

	// const int max_updates_per_frame = 5;
	const int max_updates_per_frame = 1;

	if (!isPaused)
	{
		updateRealTimeAccum += deltaTime;
		for (int i = 0; (updateRealTimeAccum >= updateTimestep) && i < max_updates_per_frame; ++i)
		{
			updateRealTimeAccum -= updateTimestep;

			//Additional IsPaused check here incase physics was paused inside one of it's components for debugging or otherwise
			if (!isPaused) UpdatePhysics();
		}

		if (updateRealTimeAccum >= updateTimestep)
		{
			NCLDebug::Log("Physics too slow to run in real time!");
			//Drop Time in the hope that it can continue to run faster the next frame
			updateRealTimeAccum = 0.0f;
		}
	}
}

void PhysicsEngine::UpdatePhysics()
{
	for (Manifold* m : manifolds)
	{
		delete m;
		m = NULL;
	}
	manifolds.clear();

	perfUpdate.UpdateRealElapsedTime(updateTimestep);
	perfBroadphase.UpdateRealElapsedTime(updateTimestep);
	perfNarrowphase.UpdateRealElapsedTime(updateTimestep);
	perfSolver.UpdateRealElapsedTime(updateTimestep);

	//A whole physics engine in 6 simple steps =D

	//-- Using positions from last frame --
	//1. Broadphase Collision Detection (Fast and dirty)
	numSphereChecks = 0;
	perfBroadphase.BeginTimingSection();
#ifdef USE_CUDA
	if (!gpuAccel)
		BroadPhaseCollisions();
#elif _WIN32
	BroadPhaseCollisions();
#endif
	perfBroadphase.EndTimingSection();

	//2. Narrowphase Collision Detection (Accurate but slow)
	perfNarrowphase.BeginTimingSection();

	if (gpuAccel)
		GPUCollisionCheck();

	NarrowPhaseCollisions();
	perfNarrowphase.EndTimingSection();

	std::random_shuffle(manifolds.begin(), manifolds.end());
	std::random_shuffle(constraints.begin(), constraints.end());

	//3. Initialize Constraint Params (precompute elasticity/baumgarte factor etc)
	//Optional step to allow constraints to 
	// precompute values based off current velocities 
	// before they are updated loop below.
	for (Manifold* m : manifolds) m->PreSolverStep(updateTimestep);
	for (Constraint* c : constraints) c->PreSolverStep(updateTimestep);

	//4. Update Velocities
	perfUpdate.BeginTimingSection();
	for (PhysicsNode* obj : physicsNodes) obj->IntegrateForVelocity(updateTimestep);
	perfUpdate.EndTimingSection();

	//5. Constraint Solver
	perfSolver.BeginTimingSection();
	for (size_t i = 0; i < SOLVER_ITERATIONS; ++i)
	{
		for (Manifold* m : manifolds) m->ApplyImpulse();
		for (Constraint* c : constraints) c->ApplyImpulse();
	}
	perfSolver.EndTimingSection();

	//6. Update Positions (with final 'real' velocities)
	perfUpdate.BeginTimingSection();
	for (PhysicsNode* obj : physicsNodes) obj->IntegrateForPosition(updateTimestep);
	perfUpdate.EndTimingSection();
}

void PhysicsEngine::BroadPhaseCollisions()
{
	broadphaseColPairs.clear();

	if (useOctree)
	{
		std::vector<PhysicsNode*> parentList;
		GenColPairs(root, parentList);
		DrawOctree(root);
	}
	else
	{
		PhysicsNode *pnodeA, *pnodeB;
		//	The broadphase needs to build a list of all potentially colliding objects in the world,
		//	which then get accurately assesed in narrowphase. If this is too coarse then the system slows down with
		//	the complexity of narrowphase collision checking, if this is too fine then collisions may be missed.

		//	Brute force approach.
		//  - For every object A, assume it could collide with every other object.. 
		//    even if they are on the opposite sides of the world.
		if (physicsNodes.size() > 0)
		{
			for (size_t i = 0; i < physicsNodes.size() - 1; ++i)
			{
				for (size_t j = i + 1; j < physicsNodes.size(); ++j)
				{
					pnodeA = physicsNodes[i];
					pnodeB = physicsNodes[j];

					//Check they both atleast have collision shapes
					if (pnodeA->GetCollisionShape() != NULL
						&& pnodeB->GetCollisionShape() != NULL)
					{
						CollisionPair cp;
						cp.pObjectA = pnodeA;
						cp.pObjectB = pnodeB;

						bool spherePass = true;
						if (sphereSphere)
						{
							Vector3 ab = cp.pObjectA->GetPosition() - cp.pObjectB->GetPosition();
							spherePass = ab.Length() <= pnodeA->GetBoundingRadius() + pnodeB->GetBoundingRadius() ? true : false;
							++numSphereChecks;
						}

						//do a coarse sphere-sphere check using bounding radii of the rendernodes
						if (spherePass)
							broadphaseColPairs.push_back(cp);
					}
				}
			}
		}
	}
}

void PhysicsEngine::ResetRoot()
{
	root = new Octree;
	for (int i = 0; i < 8; ++i)
		root->children[i] = NULL;
	root->parent = NULL;
	root->pos = Vector3(2.0f, 0.0f, 2.0f);
	//arbitrary - assign differently later (??) // - searchable token
	root->dimensions = Vector3(30.0f, 30.0f, 30.0f);
}

void PhysicsEngine::DrawOctree(Octree* tree)
{
	Vector3 corner = tree->pos - tree->dimensions;

	for (int i = 0; i < 9; ++i)
	{
		int x = i % 3;
		int y = i / 3;
		Vector3 start = corner + Vector3(tree->dimensions.x * x, tree->dimensions.y * y, 0);
		Vector3 end = start + Vector3(0, 0, tree->dimensions.z * 2);
		NCLDebug::DrawHairLineNDT(start, end, Vector4(0.0, 0.0f, 1.0f, 1.0f));
	}
	for (int i = 0; i < 9; ++i)
	{
		int y = i % 3;
		int z = i / 3;
		Vector3 start = corner + Vector3(0, tree->dimensions.y * y, tree->dimensions.z * z);
		Vector3 end = start + Vector3(tree->dimensions.x * 2, 0, 0);
		NCLDebug::DrawHairLineNDT(start, end, Vector4(1.0, 0.0f, 0.0f, 1.0f));
	}
	for (int i = 0; i < 9; ++i)
	{
		int x = i % 3;
		int z = i / 3;
		Vector3 start = corner + Vector3(tree->dimensions.x * x, 0, tree->dimensions.z * z);
		Vector3 end = start + Vector3(0, tree->dimensions.y * 2, 0);
		NCLDebug::DrawHairLineNDT(start, end, Vector4(0.0, 1.0f, 0.0f, 1.0f));
	}

	for (int i = 0; i < 8; ++i)
	{
		if (tree->children[i] != NULL) DrawOctree(tree->children[i]);
	}
}

void PhysicsEngine::GenColPairs(Octree* tree, std::vector<PhysicsNode*> parentPnodes)
{
	std::vector<PhysicsNode*> combinedList = parentPnodes;
	combinedList.insert(combinedList.end(), tree->pnodesInZone.begin(), tree->pnodesInZone.end());

	for (int i = 0; i < 8; ++i)
	{
		if (tree->children[i])
			GenColPairs(tree->children[i], combinedList);
	}
	if (tree->pnodesInZone.size() > 1)
	{
		for (size_t i = 0; i < tree->pnodesInZone.size() - 1; ++i)
		{
			for (size_t j = i + 1; j < tree->pnodesInZone.size(); ++j)
			{
				PhysicsNode *pnodeA, *pnodeB;

				pnodeA = tree->pnodesInZone[i];
				pnodeB = tree->pnodesInZone[j];

				//Check they both atleast have collision shapes
				if (pnodeA->GetCollisionShape() != NULL
					&& pnodeB->GetCollisionShape() != NULL)
				{
					CollisionPair cp;
					cp.pObjectA = pnodeA;
					cp.pObjectB = pnodeB;

					bool spherePass = true;
					if (sphereSphere)
					{
						Vector3 ab = cp.pObjectA->GetPosition() - cp.pObjectB->GetPosition();
						spherePass = ab.Length() <= pnodeA->GetBoundingRadius() + pnodeB->GetBoundingRadius() ? true : false;
						++numSphereChecks;
					}

					//do a coarse sphere-sphere check using bounding radii of the rendernodes
					if (spherePass)
						broadphaseColPairs.push_back(cp);
				}
			}
			if (parentPnodes.size() > 0)
			{
				for (size_t k = 0; k < parentPnodes.size(); ++k)
				{
					PhysicsNode *pnodeA, *pnodeB;

					pnodeA = tree->pnodesInZone[i];
					pnodeB = parentPnodes[k];

					//Check they both atleast have collision shapes
					if (pnodeA->GetCollisionShape() != NULL
						&& pnodeB->GetCollisionShape() != NULL)
					{
						CollisionPair cp;
						cp.pObjectA = pnodeA;
						cp.pObjectB = pnodeB;

						bool spherePass = true;
						if (sphereSphere)
						{
							Vector3 ab = cp.pObjectA->GetPosition() - cp.pObjectB->GetPosition();
							spherePass = ab.Length() <= pnodeA->GetBoundingRadius() + pnodeB->GetBoundingRadius() ? true : false;
							++numSphereChecks;
						}

						//do a coarse sphere-sphere check using bounding radii of the rendernodes
						if (spherePass)
							broadphaseColPairs.push_back(cp);
					}
				}
			}
		}
	}

	if (parentPnodes.size() > 0 && tree->pnodesInZone.size() > 0)
	{
		for (size_t k = 0; k < parentPnodes.size(); ++k)
		{
			PhysicsNode *pnodeA, *pnodeB;

			pnodeA = tree->pnodesInZone[tree->pnodesInZone.size() - 1];
			pnodeB = parentPnodes[k];

			//Check they both atleast have collision shapes
			if (pnodeA->GetCollisionShape() != NULL
				&& pnodeB->GetCollisionShape() != NULL)
			{
				CollisionPair cp;
				cp.pObjectA = pnodeA;
				cp.pObjectB = pnodeB;

				bool spherePass = true;
				if (sphereSphere)
				{
					Vector3 ab = cp.pObjectA->GetPosition() - cp.pObjectB->GetPosition();
					spherePass = ab.Length() <= pnodeA->GetBoundingRadius() + pnodeB->GetBoundingRadius() ? true : false;
					++numSphereChecks;
				}

				//do a coarse sphere-sphere check using bounding radii of the rendernodes
				if (spherePass)
					broadphaseColPairs.push_back(cp);
			}
		}
	}
}

void PhysicsEngine::AddToOctree(Octree* tree, PhysicsNode* pnode)
{
	Vector3 centre = tree->pos;
	Vector3 dims = tree->dimensions;

	float minSize = 0;

	if (dims.x < dims.y && dims.x < dims.z) minSize = dims.x;
	else if (dims.y < dims.z && dims.y < dims.x) minSize = dims.y;
	else minSize = dims.z;

	std::bitset<8> zones = WhichZones(tree->pos, pnode);

	if (zones.count() == 1)	//node not stradling several octants
	{
		int num = -1;
		for (int i = 0; i < 8; ++i)
			if (zones[i]) num = i;

		Octree* leaf = tree->children[num];
		if (!leaf)
		{
			int z = num / 4;
			int y = (num / 2) % 2;
			int x = num % 2;
			// pos gives the top right corner of the octant 
			Vector3 pos = centre + Vector3(x * dims.x, y * dims.y, z * dims.z);

			leaf = new Octree();
			for (int i = 0; i < 8; ++i)
				leaf->children[i] = NULL;
			leaf->parent = tree;
			tree->children[num] = leaf;
			leaf->pos = pos - (dims * 0.5);
			leaf->dimensions = dims * 0.5;
		}

		leaf->pnodesInZone.push_back(pnode);
		pnode->SetOctree(leaf);

		// if a child of the octree has too many nodes, it needs splitting
		if (leaf->pnodesInZone.size() > MAX_OBJECTS && minSize > MIN_OCTANT_SIZE)
		{
			vector<PhysicsNode*> temps;
			for (int i = leaf->pnodesInZone.size() - 1; i >= 0; --i)
			{
				temps.push_back(leaf->pnodesInZone[i]);
				leaf->pnodesInZone.pop_back();
			}
			for (int i = 0; i < temps.size(); ++i)
			{
				AddToOctree(leaf, temps[i]);
			}
			//AddToOctree(leaf, pnode);
		}
		return;
	}
	else
	{
		tree->pnodesInZone.push_back(pnode);
		pnode->SetOctree(tree);
	}
}

void PhysicsEngine::UpdateNodePosition(PhysicsNode* pnode)
{
	Octree* tree = pnode->GetOctree();

	if (!InOctree(root, pnode))
	{
		if (tree)
		{
			if (FindAndDelete(tree, pnode))
				TerminateOctree(tree);
		}
		return;
	}

	bool destroy = false;
	Octree* parent = NULL;

	// if the node doesn't currently have a tree, don't run this code
	if (tree)
	{
		parent = tree->parent;
		//checks to make sure the pnode is in the octree it thinks it is.

		destroy = FindAndDelete(tree, pnode);
	}
	else
		tree = root;

	if (parent)
	{
		if (destroy) TerminateOctree(tree);
		MoveUp(parent, pnode);
	}
	else
		AddToOctree(tree, pnode);
}

bool PhysicsEngine::FindAndDelete(Octree* tree, PhysicsNode* pnode)
{
	auto location = std::find(tree->pnodesInZone.begin(), tree->pnodesInZone.end(), pnode);
	if (location != tree->pnodesInZone.end())
	{
		tree->pnodesInZone.erase(location);
		pnode->SetOctree(NULL);

		if (tree->pnodesInZone.size() == 0)
		{
			return true;
			for (int i = 0; i < 8; ++i)
				if (tree->children[i]) return false;
		}
	}
	else
	{
		__debugbreak;
		return false;
	}
	return false;
}

void PhysicsEngine::MoveUp(Octree* tree, PhysicsNode* pnode)
{
	if (!InOctree(tree, pnode))
	{
		if (tree->parent)
		{
			Octree* parent = tree->parent;

			bool destroy = false;
			if (tree->pnodesInZone.size() == 0)
			{
				destroy = true;
				for (int i = 0; i < 8; ++i)
					if (tree->children[i]) destroy = false;
			}
			if (destroy) TerminateOctree(tree);

			MoveUp(parent, pnode);
		}
		else	// node isn't in the bounds of even the root tree
		{
			//__debugbreak;
			return;
		}
	}
	else
	{
		AddToOctree(tree, pnode);
	}
}

void PhysicsEngine::TerminateOctree(Octree* tree)
{
	for (int i = 0; i < tree->pnodesInZone.size(); ++i)
		tree->pnodesInZone[i]->SetOctree(NULL);
	for (int i = 0; i < 8; ++i)
	{
		if (tree->children[i])
			TerminateOctree(tree->children[i]);

		if (tree->parent)
			if (tree->parent->children[i] == tree)
				tree->parent->children[i] = NULL;
	}
	for (int i = 0; i < 8; ++i)
	{
		tree->children[i] = NULL;
	}
	delete tree;
	tree->parent = NULL;
	tree = NULL;
}

// must check node is COMPLETELY within a tree when moving up the tree
// assume node is in a tree when moving down and do not run this function
bool PhysicsEngine::InOctree(Octree* tree, PhysicsNode* pnode)
{
	Vector3 pos = tree->pos;
	Vector3 dims = tree->dimensions;
	Vector3 pPos = pnode->GetPosition();
	float radius = pnode->GetBoundingRadius();

	// normally make sure the node is entirely in the tree
	// if the tree is the root, we want to check to see
	// if ANY of the node is in the tree
	if (tree == root)
		radius = -radius;

	float xBounds[2] = { pos.x - dims.x, pos.x + dims.x };
	float yBounds[2] = { pos.y - dims.y, pos.y + dims.y };
	float zBounds[2] = { pos.z - dims.z, pos.z + dims.z };

	// check that the extremes of the node are entirely in the current tree
	if (pPos.x - radius > xBounds[0])
	{
		if (pPos.x + radius < xBounds[1])
		{
			if (pPos.y - radius > yBounds[0])
			{
				if (pPos.y + radius < yBounds[1])
				{
					if (pPos.z - radius > zBounds[0])
					{
						if (pPos.z + radius < zBounds[1])
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

// the return value's 8 bits tell you which octants the pnode is in
// so 11111111 is in all 8 octants of the octree
std::bitset<8> PhysicsEngine::WhichZones(Vector3 pos, PhysicsNode* pnode)
{
	unsigned char output = 0x00;
	Vector3 pPos = pnode->GetPosition();
	float radius = pnode->GetBoundingRadius();

	if (pPos.x + radius > pos.x)			// 2nd half of x
	{
		if (pPos.y + radius > pos.y)		// 2nd half of y
		{
			if (pPos.z + radius > pos.z)	// 2nd half of z
				output |= 0x80;
			if (pPos.z - radius < pos.z)	// 1st half of z
				output |= 0x08;
		}
		if (pPos.y - radius < pos.y)		// 1st half of y
		{
			if (pPos.z + radius > pos.z)	// 2nd half of z
				output |= 0x20;
			if (pPos.z - radius < pos.z)	// 1st half of z
				output |= 0x02;
		}
	}
	if (pPos.x - radius < pos.x)			// 1st half of x
	{
		if (pPos.y + radius > pos.y)		// 2nd half of y
		{
			if (pPos.z + radius > pos.z)	// 2nd half of z
				output |= 0x40;
			if (pPos.z - radius < pos.z)	// 1st half of z
				output |= 0x04;
		}
		if (pPos.y - radius < pos.y)		// 1st half of y
		{
			if (pPos.z + radius > pos.z)	// 2nd half of z
				output |= 0x10;
			if (pPos.z - radius < pos.z)	// 1st half of z
				output |= 0x01;
		}
	}

	return output;
}

void PhysicsEngine::NarrowPhaseCollisions()
{
	if (broadphaseColPairs.size() > 0)
	{
		//Collision data to pass between detection and manifold generation stages.
		CollisionData colData;

		//Collision Detection Algorithm to use
		CollisionDetectionSAT colDetect;

		// Iterate over all possible collision pairs and perform accurate collision detection
		for (size_t i = 0; i < broadphaseColPairs.size(); ++i)
		{
			CollisionPair& cp = broadphaseColPairs[i];

			CollisionShape *shapeA = cp.pObjectA->GetCollisionShape();
			CollisionShape *shapeB = cp.pObjectB->GetCollisionShape();

			PhysicsNode* pnodeA = cp.pObjectA;
			PhysicsNode* pnodeB = cp.pObjectB;

			colDetect.BeginNewPair(
				cp.pObjectA,
				cp.pObjectB,
				cp.pObjectA->GetCollisionShape(),
				cp.pObjectB->GetCollisionShape());

			//--TUTORIAL 4 CODE--
			// Detects if the objects are colliding
			if (colDetect.AreColliding(&colData))
			{
				//Note: As at the end of tutorial 4 we have very little to do, this is a bit messier
				//      than it should be. We now fire oncollision events for the two objects so they
				//      can handle AI and also optionally draw the collision normals to see roughly
				//      where and how the objects are colliding.

				//Draw collision data to the window if requested
				// - Have to do this here as colData is only temporary. 
				if (debugDrawFlags & DEBUGDRAW_FLAGS_COLLISIONNORMALS)
				{
					NCLDebug::DrawPointNDT(colData._pointOnPlane, 0.1f, Vector4(0.5f, 0.5f, 1.0f, 1.0f));
					NCLDebug::DrawThickLineNDT(colData._pointOnPlane, colData._pointOnPlane - colData._normal * colData._penetration, 0.05f, Vector4(0.0f, 0.0f, 1.0f, 1.0f));
				}

				//Check to see if any of the objects have a OnCollision callback that dont want the objects to physically collide
				bool okA = cp.pObjectA->FireOnCollisionEvent(cp.pObjectA, cp.pObjectB);
				bool okB = cp.pObjectB->FireOnCollisionEvent(cp.pObjectB, cp.pObjectA);

				if (okA && okB)
				{
					/* TUTORIAL 5 CODE */
					// Build full collision manifold that will also handle the
					// collision response between the two objects in the solver stage
					Manifold* manifold = new Manifold();

					manifold->Initiate(cp.pObjectA, cp.pObjectB);

					// Construct contact points that form the perimeter of the collision manifold

					colDetect.GenContactPoints(manifold);

					if (manifold->contactPoints.size() > 0)
					{
						// Add to list of manifolds that need solving
						manifolds.push_back(manifold);

						//Draw manifold data to the window if requested
						if (debugDrawFlags & DEBUGDRAW_FLAGS_MANIFOLD)
							manifold->DebugDraw();
					}
					else
					{
						delete manifold;
						manifold = NULL;
					}
				}
			}
		}
	}
}

void PhysicsEngine::ToggleGPUAcceleration()
{
	gpuAccel = !gpuAccel;

	if (gpuAccel)
	{
		if (!CUDA_init(physicsNodes.size() - 5))
			cout << "Error initialising CUDA memory" << endl;

		for (int i = 0; i < physicsNodes.size(); ++i)
		{

		}
	}
	else
	{
		if (!CUDA_free())
			cout << "Error freeing CUDA memory" << endl;
	}
}

void PhysicsEngine::GPUCollisionCheck()
{
	broadphaseColPairs.clear();

	int arrSize = physicsNodes.size() - 5;   //5 is the number of walls and floors and ceilings
	Vector3* positions = new Vector3[arrSize];
	float* radii = new float[arrSize];

	int index = 0;
	for (int i = 0; i < physicsNodes.size(); ++i)
	{
		PhysicsNode* pnodeA = physicsNodes[i];
		if (pnodeA->GetParent()->GetName().compare("Ground") == 0)
		{
			for (int j = 0; j < physicsNodes.size(); ++j)
			{
				if (j == i) continue;

				PhysicsNode* pnodeB = physicsNodes[j];
				//Check they both atleast have collision shapes
				if (pnodeA->GetCollisionShape() != NULL
					&& pnodeB->GetCollisionShape() != NULL)
				{
					CollisionPair cp;
					cp.pObjectA = pnodeA;
					cp.pObjectB = pnodeB;

					bool spherePass = true;
					if (sphereSphere)
					{
						Vector3 ab = cp.pObjectA->GetPosition() - cp.pObjectB->GetPosition();
						spherePass = ab.Length() <= pnodeA->GetBoundingRadius() + pnodeB->GetBoundingRadius() ? true : false;
						++numSphereChecks;
					}

					//do a coarse sphere-sphere check using bounding radii of the rendernodes
					if (spherePass)
						broadphaseColPairs.push_back(cp);
				}
			}
			continue;
		}
		if (pnodeA->GetParent()->GetName().compare("Wall") == 0)
		{
			for (int j = 0; j < physicsNodes.size(); ++j)
			{
				if (j == i) continue;

				PhysicsNode* pnodeB = physicsNodes[j];
				//Check they both atleast have collision shapes
				if (pnodeA->GetCollisionShape() != NULL
					&& pnodeB->GetCollisionShape() != NULL)
				{
					CollisionPair cp;
					cp.pObjectA = pnodeA;
					cp.pObjectB = pnodeB;

					bool spherePass = true;
					if (sphereSphere)
					{
						Vector3 ab = cp.pObjectA->GetPosition() - cp.pObjectB->GetPosition();
						spherePass = ab.Length() <= pnodeA->GetBoundingRadius() + pnodeB->GetBoundingRadius() ? true : false;
						++numSphereChecks;
					}

					//do a coarse sphere-sphere check using bounding radii of the rendernodes
					if (spherePass)
						broadphaseColPairs.push_back(cp);
				}
			}
			continue;
		}

		if (index >= arrSize)
		{
			__debugbreak;
			return;
		}

		positions[index] = pnodeA->GetPosition();
		radii[index] = pnodeA->GetBoundingRadius();

		++index;
	}

	int maxNumColPairs = arrSize * arrSize * 0.5;
	Vector3* globalOnA = new Vector3[maxNumColPairs];
	Vector3* globalOnB = new Vector3[maxNumColPairs];
	Vector3* normal = new Vector3[maxNumColPairs];
	float* penetration = new float[maxNumColPairs];
	int* indexA = new int[maxNumColPairs];
	int* indexB = new int[maxNumColPairs];

	CUDA_run(positions, radii, globalOnA, globalOnB, normal, penetration, indexA, indexB, arrSize);

	for (int i = 0; i < maxNumColPairs; ++i)
	{
		if (indexA[i] != -1)
		{
			Manifold* manifold = new Manifold;

			manifold->Initiate(physicsNodes[indexA[i] + 5], physicsNodes[indexB[i] + 5]);

			manifold->AddContact(globalOnA[i], globalOnB[i], normal[i], penetration[i]);
			if (manifold->contactPoints.size() > 0)
			{
				// Add to list of manifolds that need solving
				manifolds.push_back(manifold);
			}
			else
			{
				delete manifold;
				manifold = NULL;
			}
		}
	}

	delete[] positions;
	delete[] radii;
	delete[] globalOnA;
	delete[] globalOnB;
	delete[] normal;
	delete[] penetration;
	delete[] indexA;
	delete[] indexB;
}

void PhysicsEngine::DebugRender()
{
	// Draw all collision manifolds
	if (debugDrawFlags & DEBUGDRAW_FLAGS_MANIFOLD)
	{
		for (Manifold* m : manifolds)
		{
			m->DebugDraw();
		}
	}

	// Draw all constraints
	if (debugDrawFlags & DEBUGDRAW_FLAGS_CONSTRAINT)
	{
		for (Constraint* c : constraints)
		{
			c->DebugDraw();
		}
	}

	// Draw all associated collision shapes
	if (debugDrawFlags & DEBUGDRAW_FLAGS_COLLISIONVOLUMES)
	{
		for (PhysicsNode* obj : physicsNodes)
		{
			if (obj->GetCollisionShape() != NULL)
			{
				obj->GetCollisionShape()->DebugDraw();
			}
		}
	}
}