#pragma once
#include <string>

enum PlayerType
{
	Warlock = 0,
	Warrior,
	Rogue
};

class Player
{
public:
	Player();
	void StartTurn();
	int Attack(Player* other);
	void Block();
	bool IsAlive() const;
	void SetName(std::string& name);
	std::string GetName() const;
	int GetHP() const;
	int GetAtk() const;
	int GetDef() const;
	void Die();
protected:
	std::string name;
	int hp;
	int attack;
	int defense;
	bool isBlocking;
};