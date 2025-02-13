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
	
	void SpawnBall(const Vec3& cameraPos, const Vec3& cameraFocusPoint);

	std::vector<Body> bodies;
	std::vector<Body> nextSpawnBodies;

	static float size;
};

