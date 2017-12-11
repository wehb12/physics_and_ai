#include "DistanceConstraint.h"

void DistanceConstraint::ApplyImpulse()
{
	// Compute current constraint vars based on object A/B’s
	// position/ rotation

	Vector3 r1 = pnodeA->GetOrientation().ToMatrix3() * relPosA;
	Vector3 r2 = pnodeB->GetOrientation().ToMatrix3() * relPosB;

	// Get the global contact points in world space
	
	Vector3 globalOnA = r1 + pnodeA->GetPosition();
	Vector3 globalOnB = r2 + pnodeB->GetPosition();

	// Get the vector between the two contact points
	
	Vector3 ab = globalOnB - globalOnA;
	Vector3 abn = ab;
	
	abn.Normalise();

	// Compute the velocity of objects A and B at the point of contact

	Vector3 v0 = pnodeA->GetLinearVelocity() + Vector3::Cross(pnodeA->GetAngularVelocity(), r1);
	Vector3 v1 = pnodeB->GetLinearVelocity() + Vector3::Cross(pnodeB->GetAngularVelocity(), r2);

	// Relative velocity in constraint direction
	float abnVel = Vector3::Dot(v0 - v1, abn);
	
	// Compute the 'mass' of the constraint
	// e.g. How difficult it is to move the two objects in
	// the direction of the constraint
	
	float invConstraintMassLin = pnodeA->GetInverseMass() + pnodeB->GetInverseMass();
	
	float invConstraintMassRot = Vector3::Dot(abn,
		(Vector3::Cross(pnodeA->GetInverseInertia()
		* Vector3::Cross(r1, abn), r1)
		+ Vector3::Cross(pnodeB->GetInverseInertia()
		* Vector3::Cross(r2, abn), r2)));
	
	float constraintMass = invConstraintMassLin + invConstraintMassRot;
	
	if (constraintMass > 0.0f)
	{
		// Baumgarte Offset (Adds energy to the system to counter
		// slight solving errors that accumulate over time - known
		// as 'constraint drift')
		
		// Experiment by commenting this out and see how it
		// affects the constraints over time and when you manually

		// move the objects apart.
		
		// The key is to find a nice value that is small enough
		// not to cause objects to explode but also enough to make
		// sure all constraints /will/ be satisfied. This value
		// (0.1) will change based on your physics objects,
		// timestep etc., and also how many constraints you are
		// chaining together.
		
		float b = 0.0f;
		
		// -Optional-
		float distance_offset = ab.Length() - targetLength;
		float baumgarte_scalar = 0.1f;
		b = -(baumgarte_scalar / PhysicsEngine::Instance()->GetDeltaTime()) * distance_offset;
		// -Eof Optional-

		// Compute velocity impulse (jn)
		// In order to satisfy the distance constraint we need
		// to apply forces to ensure the relative velocity
		// (abnVel) in the direction of the constraint is zero.
		// So we take inverse of the current rel velocity and
		// multiply it by how hard it will be to move the objects.
		
		// Note: We also add in any extra energy to the system
		// here, e.g. baumgarte (and later elasticity)
		
		float jn = -(abnVel + b) / constraintMass;
		
		// Apply linear velocity impulse

		pnodeA->SetLinearVelocity(pnodeA->GetLinearVelocity() + abn * (pnodeA->GetInverseMass() * jn));
		
		pnodeB->SetLinearVelocity(pnodeB->GetLinearVelocity() - abn * (pnodeB->GetInverseMass() * jn));
		
		// Apply rotational velocity impulse

		pnodeA->SetAngularVelocity(pnodeA->GetAngularVelocity() + pnodeA->GetInverseInertia() * Vector3::Cross(r1, abn * jn));

		pnodeB->SetAngularVelocity(pnodeB->GetAngularVelocity() - pnodeB->GetInverseInertia() * Vector3::Cross(r2, abn * jn));
	}
}