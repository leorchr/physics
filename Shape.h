#pragma once
class Shape {
public:
	enum class ShapeType
	{
		SHAPE_SPHERE,
	};

	virtual ShapeType GetType() const = 0;
	virtual Vec3 GetCenterOfMass() const { return centerOfMass; }
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
	float radius;
};

