//
//  Scene.h
//
#pragma once
#include <vector>

#include "../Body.h"

/*
====================================================
Scene
====================================================
*/
class Scene {
public:
	Scene() { bodies.reserve( 128 ); }
	~Scene();

	void Reset();
	void Initialize();
	void Update( const float dt_sec );
	bool EndUpdate();
	
	void SpawnBall();

	std::vector<Body> bodies;
	std::vector<Body> nextSpawnBodies;
};

