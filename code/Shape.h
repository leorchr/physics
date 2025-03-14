#pragma once
#include "Math/Bounds.h"
#include "Math/Matrix.h"
#include "Math/Quat.h"
#include "Math/Vector.h"

class Shape {
public:
	enum class ShapeType
	{
		SHAPE_SPHERE,
	};

	virtual ShapeType GetType() const = 0;
	virtual Vec3 GetCenterOfMass() const { return centerOfMass; }
	virtual Mat3 InertiaTensor() const = 0;

	virtual Bounds GetBounds(const Vec3& pos, const Quat& orient) const = 0;
	virtual Bounds GetBounds() const = 0;
	
protected:
	Vec3 centerOfMass;
};


class ShapeSphere : public Shape {
public:
	ShapeSphere(float radiusP) : radius(radiusP)
	{
		centerOfMass.Zero();
	}
	
	ShapeType GetType() const override { return ShapeType::SHAPE_SPHERE; }
	Mat3 InertiaTensor() const override;

	Bounds GetBounds(const Vec3& pos, const Quat& orient) const override;
	Bounds GetBounds() const override;
	
	float radius;
};

