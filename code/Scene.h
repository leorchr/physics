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
extern Vec3 g_diamond[7 * 8];
void FillDiamond();

class Scene {
public:
	Scene() { bodies.reserve( 256 ); }
	~Scene();

	void Reset();
	void Initialize();
	void Update( const float dt_sec );	

	std::vector<Body> bodies;
};
