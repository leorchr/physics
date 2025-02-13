#include "Scene.h"
#include "../Shape.h"
#include "../Intersections.h"

Scene::~Scene() {
	for ( int i = 0; i < bodies.size(); i++ ) {
		delete bodies[ i ].shape;
	}
	bodies.clear();
}

void Scene::Reset() {
	for ( int i = 0; i < bodies.size(); i++ ) {
		delete bodies[ i ].shape;
	}
	bodies.clear();

	Initialize();
}

void Scene::Initialize() {
	Body body;
	body.position = Vec3(0, 0, 10);
	body.orientation = Quat(0, 0, 0, 1);
	body.shape = new ShapeSphere(1.0f);
	body.inverseMass = 1.0f;
	body.elasticity = 0.5f;
	bodies.push_back(body);
	Body earth;
	earth.position = Vec3(0, 0, -1000);
	earth.orientation = Quat(0, 0, 0, 1);
	earth.shape = new ShapeSphere(1000.0f);
	earth.inverseMass = 0.0f;
	earth.elasticity = 1.0f;
	bodies.push_back(earth);
}

void Scene::Update( const float dt_sec ) {
	for (int i = 0; i < bodies.size(); ++i) {
		Body& body = bodies[i];
		float mass = 1.0f / body.inverseMass;
		// Gravity needs to be an impulse I
		// I == dp, so F == dp/dt <=> dp = F * dt
		// <=> I = F * dt <=> I = m * g * dt
		Vec3 impulseGravity = Vec3(0, 0, -10) * mass * dt_sec;
		body.ApplyImpulseLinear(impulseGravity);
	}

	// Collision checks
	for (int i = 0; i < bodies.size(); ++i)
	{
		for (int j = i+1; j < bodies.size(); ++j)
		{
			Body& bodyA = bodies[i];
			Body& bodyB = bodies[j];
			if (bodyA.inverseMass == 0.0f && bodyB.inverseMass == 0.0f)
				continue;
			
			Contact contact;
			if (Intersections::Intersect(bodyA, bodyB, contact))
			{
				Contact::ResolveContact(contact);
			}
		}
	}
	
	// Position update
	for (int i = 0; i < bodies.size(); ++i) {
		bodies[i].Update(dt_sec);
	}
}