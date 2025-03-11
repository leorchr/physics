#pragma once

#include "Body.h"

enum class Type {
	None,
	Cochonnet,
	Boule
};


class Ball : public Body
{
public:
	Ball(Type type, class Player* player);
	~Ball();


	Type getType();
	class Player* getPlayer();


private:


	Type type;

	class Player* player = nullptr;
};

