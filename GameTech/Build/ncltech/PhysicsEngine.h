/******************************************************************************
Class: PhysicsEngine
Implements:
Author: 
	Pieran Marris      <p.marris@newcastle.ac.uk> and YOU!
Description:

	The glue that brings all our physics dreams together. This class is provided
	pretty much as is, as most of the sub-systems are tied in to this already
	with a bit of debugging code to visualise various parts of the physics engine.

	The general runtime consists of:
		- Update(float dt)
		  - UpdatePhysics()
			 - Broadphase Collision Detection
			   Quickly identifies possible collisions in between objects, hopefully
			   with world space partitioning systems like Octrees.. but currently just
			   builds a list colliding all objects with all other objects. (Hopefully you
			   can find some free time to fix this =] )

			 - Narrowphase Collision Detection
			   Takes the list provided by the broadphase collision detection and 
			   accurately collides all objects, building a collision manifold as
			   required. (Tutorial 4/5)

			 - Solve Constraints & Collisions
			   Solves all velocity constraints in the physics system, these include
			   both Collision Constraints (Tutorial 5,6) and misc world constraints
			   like distance constraints (Tutorial 3)

			 - Update Physics Objects
			   Moves all physics objects through time, updating positions/rotations
			   etc. each iteration (Tutorial 2)

*//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "PhysicsNode.h"
#include "Constraint.h"
#include "Manifold.h"
#include <nclgl\TSingleton.h>
#include <nclgl\PerfTimer.h>
#include <vector>
#include <mutex>
#include <bitset>

//Number of jacobi iterations to apply in order to
// assure the constraints are solved. (Last tutorial)
#define SOLVER_ITERATIONS 50


//Just saves including windows.h for the sake of defining true/false
#ifndef FALSE
	#define FALSE	0
	#define TRUE	1
#endif

//Debugging flags to let the engine know what to debugging information should be drawn
// - To toggle a debug draw flag on/off use <physics>->SetDebugDrawFlags(<physics>->GetDebugDrawFlags() ^ <debug flag to toggle>)"
#define DEBUGDRAW_FLAGS_CONSTRAINT				0x1
#define DEBUGDRAW_FLAGS_MANIFOLD				0x2
#define DEBUGDRAW_FLAGS_COLLISIONVOLUMES		0x4
#define DEBUGDRAW_FLAGS_COLLISIONNORMALS		0x8

//define the max number of objects per octree zone
#define MAX_OBJECTS 5
#define MIN_OCTANT_SIZE 2.0f

struct CollisionPair	//Forms the output of the broadphase collision detection
{
	PhysicsNode* pObjectA;
	PhysicsNode* pObjectB;
};

struct Octree
{
	Vector3 pos;
	Vector3 dimensions;
	std::vector<PhysicsNode*> pnodesInZone;
	Octree* children[8];
	Octree* parent;
};

class PhysicsEngine : public TSingleton<PhysicsEngine>
{
	friend class TSingleton <PhysicsEngine>;
public:
	//Reset Default Values like gravity/timestep - called when scene is switched out
	void SetDefaults();

	//Add/Remove Physics Objects
	void AddPhysicsObject(PhysicsNode* obj);
	void RemovePhysicsObject(PhysicsNode* obj);
	void RemoveAllPhysicsObjects(); //Delete all physics entities etc and reset-physics environment for new scene to be initialized

	//Add Constraints
	void AddConstraint(Constraint* c) { constraints.push_back(c); }
	
	//Update Physics Engine
	void Update(float deltaTime);			//DeltaTime here is 'seconds' since last update not milliseconds
	
	//Debug draw all physics objects, manifolds and constraints
	void DebugRender();

	//Getters / Setters 
	inline bool IsPaused() const				{ return isPaused; }
	inline void SetPaused(bool paused)			{ isPaused = paused; }

	inline uint GetDebugDrawFlags() const { return debugDrawFlags;  }
	inline void SetDebugDrawFlags(uint flags)   { debugDrawFlags = flags; }
	
	inline float GetUpdateTimestep() const { return updateTimestep; }
	inline void SetUpdateTimestep(float updateTimestep) { this->updateTimestep = updateTimestep; }

	inline const Vector3& GetGravity() const	{ return gravity; }
	inline void SetGravity(const Vector3& g)	{ gravity = g; }

	inline float GetDampingFactor() const		{ return dampingFactor; }
	inline void  SetDampingFactor(float d)		{ dampingFactor = d; }

	inline float GetDeltaTime() const			{ return updateTimestep; }

	inline void ToggleOctrees()					{ useOctree = !useOctree; }
	inline bool Octrees()						{ return useOctree; }
	void UpdateNodePosition(PhysicsNode* pnode);

	inline void ToggleSphereCheck()				{ sphereSphere = !sphereSphere; }
	inline bool SphereCheck()					{ return sphereSphere; }
	inline int NumSphereChecks()				{ return numSphereChecks; }

	void ToggleGPUAcceleration();
	inline bool GetGPUAccelerationState()		{ return gpuAccel; }

	inline size_t NumColPairs()			{ return broadphaseColPairs.size(); }

	void PrintPerformanceTimers(const Vector4& color)
	{
		perfUpdate.PrintOutputToStatusEntry(color,		"    Integration :");
		perfBroadphase.PrintOutputToStatusEntry(color,	"    Broadphase  :");
		perfNarrowphase.PrintOutputToStatusEntry(color,	"    Narrowphase :");
		perfSolver.PrintOutputToStatusEntry(color,		"    Solver      :");
	}

protected:
	PhysicsEngine();
	~PhysicsEngine();

	//The actual time-independant update function
	void UpdatePhysics();

	//Handles broadphase collision detection
	void BroadPhaseCollisions();
	//Generates Collsion pairs from an Octree
	void GenColPairs(Octree* tree, std::vector<PhysicsNode*> parentPnodes);
	void AddToOctree(Octree* tree, PhysicsNode* pnode);
	void DrawOctree(Octree* tree);
	void MoveUp(Octree* tree, PhysicsNode* pnode);
	void ResetRoot();
	bool FindAndDelete(Octree* tree, PhysicsNode* pnode);
	//delete all heap Octree structs
	void TerminateOctree(Octree* tree);
	//checks to see which zones a node is in
	std::bitset<8> WhichZones(Vector3 pos, PhysicsNode* pnode);
	bool InOctree(Octree* tree, PhysicsNode* pnode);

	//Handles narrowphase collision detection
	void NarrowPhaseCollisions();

	//Sends info to GPU to perform narrowphase collision checks on sphere-sphere collisions
	void GPUCollisionCheck();
	void InitCPUMemory();
	void FreeCPUMemory();

protected:
	bool		isPaused;
	float		updateTimestep, updateRealTimeAccum;
	uint		debugDrawFlags;

	Vector3		gravity;
	float		dampingFactor;

	std::vector<CollisionPair>  broadphaseColPairs;
	Octree*						root;
	bool						useOctree;
	bool						sphereSphere;
	int							numSphereChecks;

	bool		gpuAccel;
	Vector3* positions;
	float* radii;

	Vector3* velocityIn;
	Vector3* velocityOut;

	std::vector<PhysicsNode*>	physicsNodes;

	std::vector<Constraint*>	constraints;		// Misc constraints applying to one or more physics objects e.g our DistanceConstraint
	std::vector<Manifold*>		manifolds;			// Contact constraints between pairs of objects

	PerfTimer perfUpdate;
	PerfTimer perfBroadphase;
	PerfTimer perfNarrowphase;
	PerfTimer perfSolver;
};