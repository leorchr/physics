#pragma once
#include <string>

enum class Name {
	None,
	Player1,
	Player2
};

class Player
{
public:
	Player(Name name);
	~Player();
	Name getName();
	std::string getStringName();

	int getShootLeft();
	void setShootLeft(int number);
	bool canShoot();
	void shoot();
	int getScore();
	void setScore(int score);


private:
	Name name;

	int shootLeft = 3;

	int score = 0;
};
