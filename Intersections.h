#pragma once
#include "Body.h"
#include "Contact.h"
#include "Shape.h"

class Intersections
{
public:
	static bool Intersect(Body& a, Body& b, Contact& contact);
};