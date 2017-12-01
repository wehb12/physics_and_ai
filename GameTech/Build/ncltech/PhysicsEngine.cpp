#include "PhysicsEngine.h"
#include "GameObject.h"
#include "CollisionDetectionSAT.h"
#include <nclgl\NCLDebug.h>
#include <nclgl\Window.h>
#include <omp.h>
#include <algorithm>

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
	root = new Octree;
	for (int i = 0; i < 8; ++i)
		root->children[i] = NULL;
	root->parent = NULL;
	root->pos = Vector3(0.0f, 0.0f, 0.0f);
	//arbitrary - assign differently later (??) // - searchable token
	root->dimensions = Vector3(30.0f, 10.0f, 30.0f);

	debugDrawFlags = DEBUGDRAW_FLAGS_MANIFOLD | DEBUGDRAW_FLAGS_CONSTRAINT;

	SetDefaults();
}

PhysicsEngine::~PhysicsEngine()
{
	RemoveAllPhysicsObjects();
	TerminateOctree(root);
}

void PhysicsEngine::AddPhysicsObject(PhysicsNode* obj)
{
	physicsNodes.push_back(obj);

	AddToOctree(root, obj);
}

void PhysicsEngine::RemovePhysicsObject(PhysicsNode* obj)
{
	//Lookup the object in question
	auto found_loc = std::find(physicsNodes.begin(), physicsNodes.end(), obj);

	//If found, remove it from the list
	if (found_loc != physicsNodes.end())
	{
		physicsNodes.erase(found_loc);
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

	TerminateOctree(root);

	root = new Octree;
	for (int i = 0; i < 8; ++i)
		root->children[i] = NULL;
	root->parent = NULL;
	root->pos = Vector3(0.0f, 0.0f, 0.0f);
	//arbitrary - assign differently later (??) // - searchable token
	root->dimensions = Vector3(30.0f, 10.0f, 30.0f);
}

void PhysicsEngine::Update(float deltaTime)
{
	//The physics engine should run independantly to the renderer
	// - As our codebase is currently single threaded we just need
	//   a way of calling "UpdatePhysics()" at regular intervals
	//   or multiple times a frame if the physics timestep is higher
	//   than the renderers.
	const int max_updates_per_frame = 5;

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
	perfBroadphase.BeginTimingSection();
	BroadPhaseCollisions();
	perfBroadphase.EndTimingSection();

//2. Narrowphase Collision Detection (Accurate but slow)
	perfNarrowphase.BeginTimingSection();
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
	if (useOctree)
	{
		broadphaseColPairs.clear();

		GenColPairs(root);
		//PopulateOctree(root, physicsNodes);
		DrawOctree(root);
	}
	else
	{
		broadphaseColPairs.clear();

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
						broadphaseColPairs.push_back(cp);
					}
				}
			}
		}
	}
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

void PhysicsEngine::GenColPairs(Octree* tree)
{
	for (int x = 0; x < 2; ++x)
	{
		for (int y = 0; y < 2; ++y)
		{
			for (int z = 0; z < 2; ++z)
			{
				int num = x * 4 + y * 2 + z;

				if (tree->children[num])
					GenColPairs(tree->children[num]);
				else if (tree->pnodesInZone[num].size() > 0)
				{
					for (size_t i = 0; i < tree->pnodesInZone[num].size() - 1; ++i)
					{
						for (size_t j = i + 1; j < tree->pnodesInZone[num].size(); ++j)
						{
							PhysicsNode *pnodeA, *pnodeB;

							pnodeA = tree->pnodesInZone[num][i];
							pnodeB = tree->pnodesInZone[num][j];

							//Check they both atleast have collision shapes
							if (pnodeA->GetCollisionShape() != NULL
								&& pnodeB->GetCollisionShape() != NULL)
							{
								CollisionPair cp;
								cp.pObjectA = pnodeA;
								cp.pObjectB = pnodeB;
								broadphaseColPairs.push_back(cp);
							}
						}
					}
				}
			}
		}
	}
}

void PhysicsEngine::AddToOctree(Octree* tree, PhysicsNode* pnode, bool split)
{
	Vector3 centre = tree->pos;
	Vector3 dims = tree->dimensions;

	float minSize = 0;

	if (dims.x < dims.y && dims.x < dims.z) minSize = dims.x;
	else if (dims.y < dims.z && dims.y < dims.x) minSize = dims.y;
	else minSize = dims.z;

	for (int x = 0; x < 2; ++x)
	{
		for (int y = 0; y < 2; ++y)
		{
			for (int z = 0; z < 2; ++z)
			{
				int num = x * 4 + y * 2 + z;
				Vector3 pos = centre + Vector3(-x * dims.x, -y * dims.y, -z * dims.z);

				if (InZone(pos, dims, pnode))
				{
					// if you are resorting the tree after having to split, don't re-add the elements!
					if (!split)
						tree->pnodesInZone[num].push_back(pnode);

					if (tree->pnodesInZone[num].size() > MAX_OBJECTS && minSize > MIN_OCTANT_SIZE)
					{
						Octree* leaf = tree->children[num];
						if (!leaf)
						{
							bool split = true;
							for (int i = 0; i < 8; ++i)
								if (tree->children[i]) split = false;

							leaf = new Octree;
							for (int i = 0; i < 8; ++i)
								leaf->children[i] = NULL;
							leaf->parent = tree;
							tree->children[num] = leaf;
							leaf->pos = pos + (dims * 0.5);
							leaf->dimensions = dims * 0.5;

							if (split)
							{
								for (int i = 0; i < tree->pnodesInZone[num].size(); ++i)
								{
									AddToOctree(tree, tree->pnodesInZone[num][i], split);
								}
							}
						}
						AddToOctree(leaf, pnode);
					}
					else if (tree->pnodesInZone[num].size() > 0)
					{
						for (size_t i = 0; i < tree->pnodesInZone[num].size() - 1; ++i)
						{
							for (size_t j = i + 1; j < tree->pnodesInZone[num].size(); ++j)
							{
								PhysicsNode *pnodeA, *pnodeB;

								pnodeA = tree->pnodesInZone[num][i];
								pnodeB = tree->pnodesInZone[num][j];

								//Check they both atleast have collision shapes
								if (pnodeA->GetCollisionShape() != NULL
									&& pnodeB->GetCollisionShape() != NULL)
								{
									CollisionPair cp;
									cp.pObjectA = pnodeA;
									cp.pObjectB = pnodeB;
									broadphaseColPairs.push_back(cp);
								}
							}
						}
					}
					return;
				}
			}
		}
	}
}

void PhysicsEngine::PopulateOctree(Octree* tree, std::vector<PhysicsNode*> nodeList)
{
	Vector3 centre = tree->pos;
	Vector3 dims = tree->dimensions;

	float minSize = 0;
	if (dims.x < dims.y && dims.x < dims.z) minSize = dims.x;
	else if (dims.y < dims.z && dims.y < dims.x) minSize = dims.y;
	else minSize = dims.z;

	for (int x = 0; x < 2; ++x)
	{
		for (int y = 0; y < 2; ++y)
		{
			for (int z = 0; z < 2; ++z)
			{
				int num = x * 4 + y * 2 + z;
				Vector3 pos = centre + Vector3(-x * dims.x, -y * dims.y, -z * dims.z);

				std::vector<PhysicsNode*> pnodesInZone;
				pnodesInZone.clear();
				for (int i = 0; i < nodeList.size(); ++i)
				{
					if (InZone(pos, dims, nodeList[i]))
						pnodesInZone.push_back(nodeList[i]);
				}

				if (pnodesInZone.size() > MAX_OBJECTS && minSize > MIN_OCTANT_SIZE)
				{
					Octree* leaf = new Octree;
					for (int i = 0; i < 8; ++i)
						leaf->children[i] = NULL;
					leaf->pos = pos + (dims * 0.5);
					leaf->dimensions = dims * 0.5;
					tree->children[num] = leaf;
					PopulateOctree(leaf, pnodesInZone);
				}
				else if (pnodesInZone.size() > 0)
				{
					for (size_t i = 0; i < pnodesInZone.size() - 1; ++i)
					{
						for (size_t j = i + 1; j < pnodesInZone.size(); ++j)
						{
							PhysicsNode *pnodeA, *pnodeB;

							pnodeA = pnodesInZone[i];
							pnodeB = pnodesInZone[j];

							//Check they both atleast have collision shapes
							if (pnodeA->GetCollisionShape() != NULL
								&& pnodeB->GetCollisionShape() != NULL)
							{
								CollisionPair cp;
								cp.pObjectA = pnodeA;
								cp.pObjectB = pnodeB;
								broadphaseColPairs.push_back(cp);
							}
						}
					}
				}
			}
		}
	}
}

void PhysicsEngine::TerminateOctree(Octree* tree)
{
	for (int i = 0; i < 8; ++i)
		if (tree->children[i])
			TerminateOctree(tree->children[i]);
	delete tree;
	tree = NULL;
}

// pos is the position of the corner of the octant with the lowest (most negative)
// x, y and z coordinate. dims is the dimensions of the octant and pnode
// is the node that is being checked.
bool PhysicsEngine::InZone(Vector3 pos, Vector3 dims, PhysicsNode* pnode)
{
	float radius = 0;
	if (pnode->GetParent())
		radius = pnode->GetParent()->GetBoundingRadius();
	Vector3 pnodePos = pnode->GetPosition();

	//do a quick brute force sphere check befoe AABB
	Vector3 dir = pnode->GetPosition() - (pos + (dims * 0.5));
	if (dir.Length() < radius + (dims.Length() / 2))
	{
		if (pnodePos.x + radius > pos.x)
		{
			if (pnodePos.x - radius < pos.x + dims.x)
			{
				if (pnodePos.y + radius > pos.y)
				{
					if (pnodePos.y - radius < pos.y + dims.y)
					{
						if (pnodePos.z + radius > pos.z)
						{
							if (pnodePos.z - radius < pos.z + dims.z)
							{
								return true;
							}
						}
					}
				}
			}
		}
	}
	return false;
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

			Vector3 ab = cp.pObjectA->GetPosition() - cp.pObjectB->GetPosition();
			
			//do a coarse sphere-sphere check using bounding radii of the rendernodes
			if (ab.Length() <= pnodeA->GetBoundingRadius() + pnodeB->GetBoundingRadius())
			{
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
							{
								manifold->DebugDraw();
								//NCLDebug::DrawPolygon(manifold->contactPoints.size(), );
							}
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