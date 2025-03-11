#include "Scene.h"
#include "../Shape.h"
#include "../Intersections.h"
#include "../Broadphase.h"
#include "../Player.h"

#include <algorithm>
#include <iostream>
#include <chrono>

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

	player1 = new Player(Name::Player1);
	player2 = new Player(Name::Player2);

	currentPlayer = player1;
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


	if (IsShootFinished()) {
		if (firstShoot) {
			firstShoot = false;
		}
		else if (firstTurn) {
			currentPlayer == player1 ? currentPlayer = player2 : currentPlayer = player2;
			firstTurn = false;
		}
		else {
			CheckClosestPlayer();
		}

		PrintWhosTurn();
		currentBall = nullptr;
		canShoot = true;
		
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
	if (currentPlayer == nullptr) return;

	if (!currentPlayer->canShoot()) return;


	Vec3 dir = cameraFocusPoint - cameraPos;
	dir.Normalize();
	
	Type type = Type::None;
	float radius = 0.0f;
	if (firstShoot) {
		radius = 0.8f;
		type = Type::Cochonnet;
	}
	else {
		currentPlayer->shoot();
		radius = 1.5f;
		type = Type::Boule;
	}

	start = std::chrono::system_clock::now();
	currentBall = new Ball(type, currentPlayer);
	currentBall->position = cameraPos + dir * 20;
	currentBall->linearVelocity = dir * 75 * std::min(std::max(0.5f,strength) , 2.0f);
	currentBall->orientation = Quat(0,0,0,1);
	currentBall->shape = new ShapeSphere(radius);
	currentBall->inverseMass = 1.0f;
	currentBall->elasticity = 0.1f;
	currentBall->friction = 0.5f;



	if (firstShoot) {
		cochonnet = currentBall;
	}
	else {
		balls.push_back(currentBall);
	}

	nextSpawnBodies.push_back(std::move(*currentBall));

	canShoot = false;

}

bool Scene::IsShootFinished()
{
	if (currentBall != nullptr) {

		std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();

		std::chrono::duration<float> elapsed_seconds = end - start;

		if (elapsed_seconds.count() > 6.0f) {
			return true;
		}
	}
	return false;
}

void Scene::CheckClosestPlayer()
{
	if (cochonnet == nullptr) return;

	float minDist = std::numeric_limits<float>::max();

	Player* closestPlayer = nullptr;

	for (auto ball : balls) {
		if (ball == nullptr) continue;

		float dist = (ball->position - cochonnet->position).GetLengthSqr();
		if (dist < minDist) {
			minDist = dist;
			closestPlayer = ball->getPlayer();
		}
	}
	std::cout << "Le joueur " + closestPlayer->getStringName() << " est le plus proche !\n";

	Player* nextPlayer = nullptr;

	closestPlayer == player1 ? nextPlayer = player2 : nextPlayer = player1;

	if (nextPlayer->canShoot()) currentPlayer = nextPlayer;
	else if (closestPlayer->canShoot()) currentPlayer = closestPlayer;
	else {
		isGameFinished = true;
		winner = closestPlayer;
	}
}

void Scene::PrintWhosTurn()
{
	if (isGameFinished) {
		std::cout << winner->getStringName() << " a gagne le round !\n";
		SetWinnerScore();
		PrintScore();
		if (CheckWin()) {
			std::cout << "Partie terminee !\n";
			ResetPlayersScores();
		}
		ResetScene();
	}
	else {
		std::cout << "C'est au tour de " + currentPlayer->getStringName() << " de jouer !\n";
		std::cout << player1->getStringName() << " : " << player1->getShootLeft() << " tirs restants\n";
		std::cout << player2->getStringName() << " : " << player2->getShootLeft() << " tirs restants\n";
	}
}

void Scene::SetWinnerScore()
{
	if (cochonnet == nullptr) return;
	
	float minDist = std::numeric_limits<float>::max();

	Ball* closestBall = nullptr;

	std::vector<class Ball*> leftBalls = balls;

	for (auto ball : leftBalls) {
		if (ball == nullptr) continue;

		float dist = (ball->position - cochonnet->position).GetLengthSqr();
		if (dist < minDist) {
			minDist = dist;
			winner = ball->getPlayer();
			closestBall = ball;
		}
	}
	int score = winner->getScore();
	score++;

	leftBalls.erase(std::find(std::begin(leftBalls), std::end(leftBalls), closestBall));


	while (!leftBalls.empty()) {
		Player* closestPlayer = nullptr;
		Ball* newClosestBall = nullptr;

		for (auto ball : leftBalls) {
			if (ball == nullptr) continue;

			float dist = (ball->position - cochonnet->position).GetLengthSqr();
			if (dist < minDist) {
				minDist = dist;
				closestPlayer = ball->getPlayer();
				newClosestBall = ball;
			}
		}

		if (closestPlayer != winner) leftBalls.clear();
		else {
			score++;
			leftBalls.erase(std::find(std::begin(leftBalls), std::end(leftBalls), newClosestBall));
		}
	}

	winner->setScore(score);
}

void Scene::PrintScore()
{
	std::cout << player1->getStringName() << " : " << player1->getScore() << " points\n";
	std::cout << player2->getStringName() << " : " << player2->getScore() << " points\n";
}

bool Scene::CheckWin()
{
	if (player1 == nullptr) return false;
	if (player2 == nullptr) return false;

	if (player1->getScore() >= 13) return true;
	else if (player2->getScore() >= 13) return true;
	else { return false; }
}

void Scene::ResetPlayersScores()
{
	if (player1 == nullptr) return;
	if (player2 == nullptr) return;

	player1->setScore(0);
	player2->setScore(0);
}

void Scene::ResetScene()
{
	player1->setShootLeft(3);
	player2->setShootLeft(3);

	currentPlayer = winner;

	if (cochonnet != nullptr) delete cochonnet;
	
	for (auto ball : balls) {
		if (ball != nullptr) delete ball;
	}
	balls.clear();

	currentPlayer = nullptr;
	winner = nullptr;
	currentBall = nullptr;

	canShoot = true;
	firstShoot = true;
	firstTurn = true;
	isGameFinished = false;
}

float Scene::size = 0.4f;