#include "Manifold.h"
#include <nclgl\Matrix3.h>
#include <nclgl\NCLDebug.h>
#include "PhysicsEngine.h"
#include <algorithm>

Manifold::Manifold() 
	: pnodeA(NULL)
	, pnodeB(NULL)
{
}

Manifold::~Manifold()
{

}

void Manifold::Initiate(PhysicsNode* nodeA, PhysicsNode* nodeB)
{
	contactPoints.clear();

	pnodeA = nodeA;
	pnodeB = nodeB;
}

void Manifold::ApplyImpulse()
{
	for (ContactPoint& contact : contactPoints)
	{
		SolveContactPoint(contact);
	}
}


void Manifold::SolveContactPoint(ContactPoint& c)
{	
	/* TUTORIAL 6 CODE */
}

void Manifold::PreSolverStep(float dt)
{
	for (ContactPoint& contact : contactPoints)
	{
		UpdateConstraint(contact);
	}
}

void Manifold::UpdateConstraint(ContactPoint& c)
{
	//Reset total impulse forces computed this physics timestep 
	c.sumImpulseContact = 0.0f;
	c.sumImpulseFriction = Vector3(0.0f, 0.0f, 0.0f);
	c.b_term = 0.0f;

	/* TUTORIAL 6 CODE */
}

void Manifold::AddContact(const Vector3& globalOnA, const Vector3& globalOnB, const Vector3& normal, const float& penetration)
{
	//Get relative offsets from each object centre of mass
	// Used to compute rotational velocity at the point of contact.
	Vector3 r1 = (globalOnA - pnodeA->GetPosition());
	Vector3 r2 = (globalOnB - pnodeB->GetPosition());

	//Create our new contact descriptor
	ContactPoint contact;
	contact.relPosA = r1;
	contact.relPosB = r2;
	contact.colNormal = normal;
	contact.colNormal.Normalise();
	contact.colPenetration = penetration;

	contactPoints.push_back(contact);


	//What a stupid function!
	// - Manifold's normally persist over multiple frames, as in two colliding objects
	//   (especially in the case of stacking) will likely be colliding in a similar 
	//   setup the following couple of frames. So the accuracy therefore can be increased
	//   by using persistent manifolds, which will only be deleted when the two objects
	//   fail a narrowphase check. This means the manifolds can be quite busy, with lots of
	//   new contacts per frame, but to solve any convex manifold in 3D you really only need
	//   3 contacts (4 max), so tldr; perhaps you want persistent manifolds.. perhaps
	//   you want to put some code here to sort contact points.. perhaps this comment is even 
	//   more pointless than a passthrough function.. perhaps I should just stop typ
}

void Manifold::DebugDraw() const
{
	if (contactPoints.size() > 0)
	{
		//Loop around all contact points and draw them all as a line-loop
		Vector3 globalOnA1 = pnodeA->GetPosition() + contactPoints.back().relPosA;
		for (const ContactPoint& contact : contactPoints)
		{
			Vector3 globalOnA2 = pnodeA->GetPosition() + contact.relPosA;
			Vector3 globalOnB = pnodeB->GetPosition() + contact.relPosB;

			//Draw line to form area given by all contact points
			NCLDebug::DrawThickLineNDT(globalOnA1, globalOnA2, 0.02f, Vector4(0.0f, 1.0f, 0.0f, 1.0f));

			//Draw descriptors for indivdual contact point
			NCLDebug::DrawPointNDT(globalOnA2, 0.05f, Vector4(0.0f, 0.5f, 0.0f, 1.0f));
			NCLDebug::DrawThickLineNDT(globalOnB, globalOnA2, 0.01f, Vector4(1.0f, 0.0f, 1.0f, 1.0f));

			globalOnA1 = globalOnA2;
		}
	}
}