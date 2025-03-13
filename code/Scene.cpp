#include "Scene.h"
#include "Shape.h"
#include "Intersections.h"
#include "Broadphase.h"
#include "Player.h"

#include <algorithm>
#include <iostream>
#include <chrono>

Scene::~Scene() {
	for ( int i = 0; i < bodies.size(); i++ ) {
		delete bodies[ i ]->shape;
	}
	bodies.clear();
}

void Scene::Reset() {
	for ( int i = 0; i < bodies.size(); i++ ) {
		if (bodies[i]->shape != nullptr) delete bodies[i]->shape;
	}
	bodies.clear();

	Initialize();
}

void Scene::Initialize() {
	
	float radius = 500.0f;
	earth = new Body();
	earth->position = Vec3(0, 0, -radius);
	earth->orientation = Quat(0, 0, 0, 1);
	earth->shape = new ShapeSphere(radius);
	earth->inverseMass = 0.0f;
	earth->elasticity = 0.2f;
	earth->friction = 0.5f;
	bodies.push_back(earth);

	if(player1 == nullptr) player1 = new Player(Name::Player1);
	if(player2 == nullptr) player2 = new Player(Name::Player2);

	if (winner == nullptr) currentPlayer = player1;
	else currentPlayer = winner;
}

void Scene::Update(const float dt_sec)
{
	// Gravity
	for (int i = 0; i < bodies.size(); ++i)
	{
		Body& body = *bodies[i];
		float mass = 1.0f / body.inverseMass;
		// Gravity needs to be an impulse I
		// I == dp, so F == dp/dt <=> dp = F * dt
		// <=> I = F * dt <=> I = m * g * dt
		Vec3 centerOfGravity = earth->position - body.position;
		centerOfGravity.Normalize();
		centerOfGravity *= 9.8f;

		Vec3 impulseGravity = centerOfGravity * mass * dt_sec;
		body.ApplyImpulseLinear(impulseGravity);

		body.linearVelocity = Vec3::Lerp(body.linearVelocity, Vec3(0, 0, 0), 0.01f);
		body.angularVelocity = Vec3::Lerp(body.angularVelocity, Vec3(0, 0, 0), 0.01f);
	}
	// Broadphase
	std::vector<CollisionPair> collisionPairs;
	BroadPhase(bodies, collisionPairs, dt_sec);
	// Collision checks (Narrow phase)
	int numContacts = 0;
	const int maxContacts = bodies.size() * bodies.size();
	Contact* contacts = (Contact*)alloca(sizeof(Contact) * maxContacts);
	for (int i = 0; i < collisionPairs.size(); ++i)
	{
		const CollisionPair& pair = collisionPairs[i];
		Body& bodyA = *bodies[pair.a];
		Body& bodyB = *bodies[pair.b];
		if (bodyA.inverseMass == 0.0f && bodyB.inverseMass == 0.0f)
			continue;
		Contact contact;
		if (Intersections::Intersect(bodyA, bodyB, dt_sec, contact))
		{
			contacts[numContacts] = contact;
			++numContacts;
			bodyA.linearVelocity = Vec3::Lerp(bodyA.linearVelocity, Vec3(0, 0, 0), 0.015);
			bodyA.angularVelocity = Vec3::Lerp(bodyA.angularVelocity, Vec3(0, 0, 0), 0.015);
			bodyB.linearVelocity = Vec3::Lerp(bodyB.linearVelocity, Vec3(0, 0, 0), 0.015);
			bodyB.angularVelocity = Vec3::Lerp(bodyB.angularVelocity, Vec3(0, 0, 0), 0.015);
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
			bodies[j]->Update(dt);
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
			bodies[i]->Update(timeRemaining);
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
			bodies.push_back(body);
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
		radius = 0.6f;
		type = Type::Cochonnet;
	}
	else {
		currentPlayer->shoot();
		radius = 1.2f;
		type = Type::Boule;
	}

	start = std::chrono::system_clock::now();
	currentBall = new Ball(type, currentPlayer);
	currentBall->position = cameraPos + dir * 20;
	dir.z += 0.1f;
	currentBall->linearVelocity = dir * 35 * std::min(std::max(0.8f,strength) , 1.5f);
	currentBall->orientation = Quat(0,0,0,1);
	currentBall->shape = new ShapeSphere(radius);
	currentBall->inverseMass = 3.0f;
	currentBall->elasticity = 0.1f;
	currentBall->friction = 0.5f;



	if (firstShoot) {
		cochonnet = currentBall;
	}
	else {
		balls.push_back(currentBall);
	}

	nextSpawnBodies.push_back(currentBall);

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
	std::cout << closestPlayer->getStringName() << " is the closest to the cochonnet.\n" << std::flush;

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
		std::cout << winner->getStringName() << " win this round !\n";
		std::cout << "===================================\n";
		SetWinnerScore();
		PrintScore();
		if (CheckWin()) {
			std::cout << "End of the game !\n";
			ResetPlayersScores();
		}
		ResetScene();
	}
	else {
		std::cout << player1->getStringName() << " : " << player1->getShootLeft() << " shot" << (player1->getShootLeft() != 1 ? "s" : "") << " remaining\n";
		std::cout << player2->getStringName() << " : " << player2->getShootLeft() << " shot" << (player2->getShootLeft() != 1 ? "s" : "") << " remaining\n";

		std::cout << "It's " + currentPlayer->getStringName() << "'s turn.\n";
		std::cout << "===================================\n" << std::flush;
		turn++;
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

	leftBalls.erase(std::remove(leftBalls.begin(), leftBalls.end(), closestBall), leftBalls.end());


	while (!leftBalls.empty()) {

		minDist = std::numeric_limits<float>::max();
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
			leftBalls.erase(std::remove(leftBalls.begin(), leftBalls.end(), newClosestBall), leftBalls.end());
		}
	}

	winner->setScore(score);
}

void Scene::PrintScore()
{
	std::cout << player1->getStringName() << " : " << player1->getScore() << " points\n";
	std::cout << player2->getStringName() << " : " << player2->getScore() << " points\n" << std::flush;
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

	Reset();

	cochonnet = nullptr;
	balls.clear();

	winner = nullptr;
	currentBall = nullptr;

	canShoot = true;
	firstShoot = true;
	firstTurn = true;
	isGameFinished = false;
	turn = 0;
}

void Scene::SetColor(const std::string& color)
{
	std::cout << color;
}

void Scene::ExplainRules()
{
	std::cout << std::flush;
	SetColor("\033[1;34m");
	std::cout << "Rules of Petanque\n";
	SetColor("\033[0m");

	std::cout << "\nPetanque is a very popular game in France. Here are the basic rules:\n\n";

	SetColor("\033[1;33m");
	std::cout << "1. Objective of the game:\n";
	SetColor("\033[0m");
	std::cout << "   The goal is to throw metal balls (called boules) as close as possible to a small target called the 'cochonnet'.\n\n";

	SetColor("\033[1;33m");
	std::cout << "2. Number of players:\n";
	SetColor("\033[0m");
	std::cout << "   Petanque is typically played with 2 players.\n\n";

	SetColor("\033[1;33m");
	std::cout << "3. Game procedure:\n";
	SetColor("\033[0m");
	std::cout << "   The game starts with a coin toss to decide who will throw the cochonnet.\nThe first player throws a boule to place it at a reasonable distance.\n";
	std::cout << "   Then, players take turns throwing their boules to try to get closer to the cochonnet.\nThe player closest to the cochonnet has the advantage.\n\n";

	SetColor("\033[1;33m");
	std::cout << "4. Scoring:\n";
	SetColor("\033[0m");
	std::cout << "   Once all the boules are thrown, the team with the boule closest to the cochonnet scores a point.\nAny other boules closer than the opponent's also count.\n";
	std::cout << "   The team that scores points then throws the cochonnet for the next round.\n\n";

	SetColor("\033[1;33m");
	std::cout << "5. End of the game:\n";
	SetColor("\033[0m");
	std::cout << "   The game continues until a team reaches a certain number of points, usually 13.\n\n";

	SetColor("\033[1;32m");
	std::cout << "Have fun and may the best team win!\n\n";
	SetColor("\033[0m");
	std::cout << "===================================\n" << std::flush;
}

float Scene::size = 0.4f;