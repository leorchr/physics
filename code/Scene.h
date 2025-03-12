//
//  Scene.h
//
#pragma once
#include "application.h"
#include <vector>
#include <chrono>
#include <string>

#include "Ball.h"

/*
====================================================
Scene
====================================================
*/
class Scene {
public:
	Scene() { bodies.reserve( 128 ); nextSpawnBodies.reserve(128);}
	~Scene();

	void Reset();
	void Initialize();
	void Update( const float dt_sec );
	bool EndUpdate();
	
	void SpawnBall(const Vec3& cameraPos, const Vec3& cameraFocusPoint, float strength);

	std::vector<Body*> bodies;
	std::vector<Body*> nextSpawnBodies;
	bool IsShootFinished();
	void CheckClosestPlayer();
	void PrintWhosTurn();

	void SetWinnerScore();
	void PrintScore();
	bool CheckWin();
	void ResetPlayersScores();
	void ResetScene();

	void SetColor(const std::string& color);
	void ExplainRules();



private:
	class Body* earth = nullptr;

	class Player* player1 = nullptr;
	class Player* player2 = nullptr;
	class Player* currentPlayer = nullptr;
	class Player* winner = nullptr;

	class Ball* cochonnet = nullptr;
	std::vector<class Ball*> balls;

	class Ball* currentBall = nullptr;

	bool canShoot = true;
	bool firstShoot = true;
	bool firstTurn = true;
	bool isGameFinished = false;

	std::chrono::time_point<std::chrono::system_clock> start;

	static float size;

	int turn = 1;
};

