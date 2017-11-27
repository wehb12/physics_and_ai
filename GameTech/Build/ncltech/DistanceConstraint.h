/******************************************************************************
Class: DistanceConstraint
Implements:
Author: 
	Pieran Marris      <p.marris@newcastle.ac.uk> and YOU!
Description:

	Manages a distance constraint between two objects, ensuring the two objects never
	seperate. It works on the velocity level, enforcing the constraint:
		dot([(velocity of B) - (velocity of A)], normal) = zero
	
	Which is the same as saying, if the velocity of the two objects in the direction of the constraint is zero
	then we can assert that the two objects will not move further or closer together and thus satisfy the constraint.

*//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Constraint.h"
#include "PhysicsEngine.h"
#include <nclgl\NCLDebug.h>

class DistanceConstraint : public Constraint
{
public:
	DistanceConstraint(PhysicsNode* obj1, PhysicsNode* obj2,
		const Vector3& globalOnA, const Vector3& globalOnB)
	{
		pnodeA = obj1;
		pnodeB = obj2;

		//Set the preferred distance of the constraint to enforce 
		// (ONLY USED FOR BAUMGARTE)
		// - Ideally we only ever work at the velocity level, so satifying (velA-VelB = 0)
		//   is enough to ensure the distance never changes.
		Vector3 ab = globalOnB - globalOnA;
		targetLength = ab.Length();

		//Get the local points (un-rotated) on the two objects where the constraint should
		// be attached relative to the objects center. So when we rotate the objects
		// the constraint attachment point will rotate with it.
		Vector3 r1 = (globalOnA - pnodeA->GetPosition());
		Vector3 r2 = (globalOnB - pnodeB->GetPosition());
		relPosA = Matrix3::Transpose(pnodeA->GetOrientation().ToMatrix3()) * r1;
		relPosB = Matrix3::Transpose(pnodeB->GetOrientation().ToMatrix3()) * r2;
	}

	//Solves the constraint and applies a velocity impulse to the two
	// objects in order to satisfy the constraint.
	virtual void ApplyImpulse() override
	{
	
	}

	//Draw the constraint visually to the screen for debugging
	virtual void DebugDraw() const
	{
		Vector3 globalOnA = pnodeA->GetOrientation().ToMatrix3() * relPosA + pnodeA->GetPosition();
		Vector3 globalOnB = pnodeB->GetOrientation().ToMatrix3() * relPosB + pnodeB->GetPosition();

		NCLDebug::DrawThickLine(globalOnA, globalOnB, 0.02f, Vector4(0.0f, 0.0f, 0.0f, 1.0f));
		NCLDebug::DrawPointNDT(globalOnA, 0.05f, Vector4(1.0f, 0.8f, 1.0f, 1.0f));
		NCLDebug::DrawPointNDT(globalOnB, 0.05f, Vector4(1.0f, 0.8f, 1.0f, 1.0f));
	}

protected:
	PhysicsNode *pnodeA, *pnodeB;

	float   targetLength;

	Vector3 relPosA;
	Vector3 relPosB;
};