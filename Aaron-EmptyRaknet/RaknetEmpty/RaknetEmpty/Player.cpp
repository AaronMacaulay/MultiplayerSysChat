#include "Player.h"

Player::Player()
{
	isBlocking = false;
}

void Player::StartTurn()
{
	isBlocking = false;
}

int Player::Attack(Player* other)
{
	int damage;
	damage = (float)attack * TypeMultiplier(other);
	if (other->isBlocking)
	{
		damage = damage - other->defense;
	}
	else
	{
		damage = (damage * 3 - other->defense) / 3;
	}
	if (damage <= 0)
	{
		damage = 1;
	}
	other->hp -= damage;
	return damage;
}

void Player::Block()
{
	isBlocking = true;
}

bool Player::IsAlive() const
{
	return hp > 0;
}

void Player::SetName(std::string& nm)
{
	name = nm;
}

std::string Player::GetName() const
{
	return name;
}

int Player::GetHP() const
{
	return hp;
}

int Player::GetAtk() const
{
	return attack;
}

int Player::GetDef() const
{
	return defense;
}

void Player::Die()
{
	hp = 0;
}