#pragma once
#include "InputManager.h"
#include "Player.h"

struct BattleData;
class InputManager;
class Player;

class BattleInputManager
{
public:
	// 更新処理
	bool Update(BattleData& data, 
		const InputManager& input, 
		Player& humanPlayer, 
		int humanIdx, 
		bool isHumanTurn);
		
private:
	// 降参（サレンダー）関連のUI処理
	bool ProcessSurrender(BattleData& data, const InputManager& input);

	// 攻撃・防御の決定ボタン処理
	void ProcessActionButtons(BattleData& data, const InputManager& input, int humanIdx, bool isHumanTurn);

	// マニュアルでのターゲット選択処理（ステータスUIクリック）
	void ProcessTargetSelection(BattleData& data, const InputManager& input, bool isHumanTurn);

	// 手札のホバー・クリック（選択とコンボ）処理
	void ProcessHandSelection(BattleData& data, const InputManager& input, Player& humanPlayer, int humanIdx, bool isHumanTurn);
};

