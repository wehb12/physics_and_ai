#include "PhysicsNode.h"
#include "PhysicsEngine.h"


void PhysicsNode::IntegrateForVelocity(float dt)
{
	if (invMass > 0.0f)
	{
		linVelocity += PhysicsEngine::Instance()->GetGravity() * dt;
	}
	//Semi-Implicit Euler Method
	/*
	linVelocity += force * invMass * dt;
	angVelocity += invInertia * torque * dt;
	*/

	//RK2 method
	Vector3 linVelocity_p1 = linVelocity + force * invMass * dt;
	linVelocity = (linVelocity + linVelocity_p1) * 0.5;
	Vector3 angVelocity_p1 = invInertia * angVelocity + torque * dt;
	angVelocity = (angVelocity + angVelocity_p1) * 0.5;

	linVelocity = linVelocity * PhysicsEngine::Instance()->GetDampingFactor();
	angVelocity = angVelocity * PhysicsEngine::Instance()->GetDampingFactor();

	//if (linVelocity != Vector3(0, 0, 0))
	//	PhysicsEngine::Instance()->UpdateNodePosition(this);
}

/* Between these two functions the physics engine will solve for velocity
   based on collisions/constraints etc. So we need to integrate velocity, solve 
   constraints, then use final velocity to update position. 
*/

void PhysicsNode::IntegrateForPosition(float dt)
{
	position += linVelocity * dt;
	orientation = orientation + Quaternion(angVelocity * dt * 0.5f, 0.0f) * orientation;
	orientation.Normalise();

	distMoved += linVelocity * dt;
	//Finally: Notify any listener's that this PhysicsNode has a new world transform.
	// - This is used by GameObject to set the worldTransform of any RenderNode's. 
	//   Please don't delete this!!!!!
	FireOnUpdateCallback();
}

void PhysicsNode::FireOnUpdateCallback()
{
	//Build world transform
	worldTransform = orientation.ToMatrix4();
	worldTransform.SetPositionVector(position);

	//Fire the OnUpdateCallback, notifying GameObject's and other potential
	// listeners that this PhysicsNode has a new world transform.
	if (onUpdateCallback) onUpdateCallback(worldTransform);

	float deltaLength = 0.01f;
	if (octree)
		deltaLength = 0.01 * octree->dimensions.Length() / boundingRadius;

	if (distMoved.Length() > deltaLength)
	{
		PhysicsEngine::Instance()->UpdateNodePosition(this);
		distMoved.ToZero();
	}
}