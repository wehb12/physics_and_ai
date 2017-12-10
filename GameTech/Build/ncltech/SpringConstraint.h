/******************************************************************************
Class: SpringConstraint
Implements:
Author:
Will Hinds      <w.hinds2@newcastle.ac.uk>
Description:

A constraint that applies a force F in proportion to the displacement of the constraint x
such that F = k * x, where k is the spring constant. The force is applied in the direction
opposite to the displacement

*//////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef GRAVITY
#define GRAVITY 9.81f
#endif

#include "Constraint.h"
#include "GameObject.h"
#include "PhysicsEngine.h"
#include <nclgl\NCLDebug.h>

class SpringConstraint : public Constraint
{
public:
	// This is a specific case where a spring constraint between a point and
	// an object is set up
	SpringConstraint(PhysicsNode* pnode, Vector3 point,
		const Vector3& globalOnA, float k, float c)
	{
		pnodeA = pnode;
		orientationA = pnodeA->GetOrientation();
		//pnodeB = NULL;
		pointPos = point;

		springConst = k;
		dampingFactor = c;

		Vector3 ap = point - globalOnA;
		targetLength = ap.Length();

		Vector3 r1 = (globalOnA - pnodeA->GetPosition());
		relPosA = Matrix3::Transpose(pnodeA->GetOrientation().ToMatrix3()) * r1;

		//relPosB = point;
	}

	//Solves the constraint and applies a velocity impulse to the two
	// objects in order to satisfy the constraint.
	virtual void ApplyImpulse() override;

	//Draw the constraint visually to the screen for debugging
	virtual void DebugDraw() const
	{
		Vector3 globalOnA = pnodeA->GetOrientation().ToMatrix3() * relPosA + pnodeA->GetPosition();
		Vector3 globalOnB;
		//if (pnodeB)
		//	globalOnB = pnodeB->GetOrientation().ToMatrix3() * relPosB + pnodeB->GetPosition();
		//else
			globalOnB = pointPos;

		// make this a spring shaped traingle line (??)
		NCLDebug::DrawThickLine(globalOnA, globalOnB, 0.02f, Vector4(0.0f, 0.0f, 0.0f, 1.0f));
		NCLDebug::DrawPointNDT(globalOnA, 0.05f, Vector4(1.0f, 0.8f, 1.0f, 1.0f));
		NCLDebug::DrawPointNDT(globalOnB, 0.05f, Vector4(1.0f, 0.8f, 1.0f, 1.0f));
	}

protected:
	PhysicsNode *pnodeA;//, *pnodeB;
	Vector3 pointPos;
	Quaternion orientationA;

	float   targetLength;

	Vector3 relPosA;
	//Vector3 relPosB;

	float springConst;
	float dampingFactor;
};