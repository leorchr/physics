#include "Body.h"
#include "Shape.h"

Vec3 Body::GetCenterOfMassWorldSpace() const
{
	const Vec3 centerOfMass = shape->GetCenterOfMass();
	const Vec3 pos = position + orientation.RotatePoint(centerOfMass);
	return pos;
}

Vec3 Body::GetCenterOfMassBodySpace() const
{
	return shape->GetCenterOfMass();
}

Vec3 Body::WorldSpaceToBodySpace(const Vec3& worldPoint)
{
	const Vec3 tmp = worldPoint - GetCenterOfMassWorldSpace();
	const Quat invertOrient = orientation.Inverse();
	Vec3 bodySpace = invertOrient.RotatePoint(tmp);
	return bodySpace;
}

Vec3 Body::BodySpaceToWorldSpace(const Vec3& bodyPoint)
{
	Vec3 worldSpace = GetCenterOfMassWorldSpace()

	+ orientation.RotatePoint(bodyPoint);

	return worldSpace;
}

void Body::ApplyImpulseLinear(const Vec3& impulse)
{
	if (inverseMass == 0.0f) return;
	// dv = J / m
	linearVelocity += impulse * inverseMass;
}