#include "Scene.h"
#include "../Shape.h"
#include "../Intersections.h"
#include "../Broadphase.h"
#include "../Player.h"

#include <algorithm>
#include <iostream>

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
	
	float radius = 500.0f;
	earth.position = Vec3(0, 0, -radius);
	earth.orientation = Quat(0, 0, 0, 1);
	earth.shape = new ShapeSphere(radius);
	earth.inverseMass = 0.0f;
	earth.elasticity = 0.2f;
	earth.friction = 0.5f;
	bodies.push_back(earth);
}

void Scene::Update(const float dt_sec)
{
	// Gravity
	for (int i = 0; i < bodies.size(); ++i)
	{
		Body& body = bodies[i];
		float mass = 1.0f / body.inverseMass;
		// Gravity needs to be an impulse I
		// I == dp, so F == dp/dt <=> dp = F * dt
		// <=> I = F * dt <=> I = m * g * dt
		Vec3 centerOfGravity = earth.position - body.position;
		centerOfGravity.Normalize();
		centerOfGravity *= 9.8f;

		Vec3 impulseGravity = centerOfGravity * mass * dt_sec;
		body.ApplyImpulseLinear(impulseGravity);

		
		body.linearVelocity = Vec3::Lerp(body.linearVelocity, Vec3(0, 0, 0), 0.01);
		body.angularVelocity = Vec3::Lerp(body.angularVelocity, Vec3(0, 0, 0), 0.01);
	}
	// Broadphase
	std::vector<CollisionPair> collisionPairs;
	BroadPhase(bodies.data(), bodies.size(), collisionPairs, dt_sec);
	// Collision checks (Narrow phase)
	int numContacts = 0;
	const int maxContacts = bodies.size() * bodies.size();
	Contact* contacts = (Contact*)alloca(sizeof(Contact) * maxContacts);
	for (int i = 0; i < collisionPairs.size(); ++i)
	{
		const CollisionPair& pair = collisionPairs[i];
		Body& bodyA = bodies[pair.a];
		Body& bodyB = bodies[pair.b];
		if (bodyA.inverseMass == 0.0f && bodyB.inverseMass == 0.0f)
			continue;
		Contact contact;
		if (Intersections::Intersect(bodyA, bodyB, dt_sec, contact))
		{
			contacts[numContacts] = contact;
			++numContacts;
		}
	}
	// Sort times of impact
	if (numContacts > 1) {
		qsort(contacts, numContacts, sizeof(Contact),
		Contact::CompareContact);
	}
	// Contact resolve in order
	
	float accumulatedTime = 0.0f;
	for (int i = 0; i < numContacts; ++i)
	{
		Contact& contact = contacts[i];
		const float dt = contact.timeOfImpact - accumulatedTime;
		Body* bodyA = contact.a;
		Body* bodyB = contact.b;
		// Skip body par with infinite mass
		if (bodyA->inverseMass == 0.0f && bodyB->inverseMass == 0.0f)
			continue;
		// Position update
		for (int j = 0; j < bodies.size(); ++j) {
			bodies[j].Update(dt);
		}
		Contact::ResolveContact(contact);
		accumulatedTime += dt;
	}
	// Other physics behavirous, outside collisions.
	// Update the positions for the rest of this frame's time.
	const float timeRemaining = dt_sec - accumulatedTime;
	if (timeRemaining > 0.0f)
	{
		for (int i = 0; i < bodies.size(); ++i) {
			bodies[i].Update(timeRemaining);
		}
	}

	if (currentBall != nullptr) {
		float velocityLength = std::abs(currentBall->linearVelocity.GetLengthSqr());
		if (std::abs(currentBall->linearVelocity.GetLengthSqr()) < 1406.26f) {
			std::cout << "Je suis proche de 0\n" << std::flush;
		}
	}
}

bool Scene::EndUpdate()
{
	if (!std::empty(nextSpawnBodies))
	{
		for (auto& body : nextSpawnBodies)
		{
			bodies.push_back(std::move(body));
		}
		nextSpawnBodies.clear(); // Clear the original list after moving
		return true;
	}
	return false;
}


void Scene::SpawnBall(const Vec3& cameraPos, const Vec3& cameraFocusPoint, float strength)
{
	if (!canShoot) return;

	Vec3 dir = cameraFocusPoint - cameraPos;
	dir.Normalize();
	
	float radius = 0.0f;
	if (firstShoot) {
		radius = 0.8f;
	}
	else {
		radius = 1.5f;
	}
	currentBall = new Body();
	currentBall->position = cameraPos + dir * 20;
	currentBall->linearVelocity = dir * 75 * std::min(std::max(0.5f,strength) , 2.0f);
	currentBall->orientation = Quat(0,0,0,1);
	currentBall->shape = new ShapeSphere(radius);
	currentBall->inverseMass = 1.0f;
	currentBall->elasticity = 0.1f;
	currentBall->friction = 0.5f;
	nextSpawnBodies.push_back(*currentBall);


	if (firstShoot) {
		cochonnet = currentBall;
	}
	else {
		balls.push_back(currentBall);
	}

}

float Scene::size = 0.4f;