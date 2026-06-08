#pragma once
#include "InputManager.h"
#include "Player.h"

struct BattleData;
class InputManager;
class Player;

class BattleInputManager
{
public:
	// XVˆ—
	bool Update(BattleData& data, 
		const InputManager& input, 
		Player& humanPlayer, 
		int humanIdx, 
		bool isHumanTurn);
		
};

