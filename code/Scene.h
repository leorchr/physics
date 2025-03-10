//
//  Scene.h
//
#pragma once
#include "application.h"
#include <vector>

#include "../Body.h"

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

	std::vector<Body> bodies;
	std::vector<Body> nextSpawnBodies;

	Body earth;

	class Player* player1;
	class Player* player2;
	class Player* currentPlayer;

	class Body* cochonnet;
	std::vector<class Body*> balls;

	class Body* currentBall = nullptr;

	bool canShoot = true;
	bool firstShoot = true;

	static float size;
};

