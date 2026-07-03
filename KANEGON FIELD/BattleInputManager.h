#pragma once
#include "InputManager.h"
#include "Player.h"

struct BattleData;
class InputManager;
class Player;

// プレイヤーの「行動」をまとめる構造体
struct PlayerAction {
	bool hasAction = false;         // 何かしらの行動を決定したか
	bool isSurrender = false;       // 降参ボタンを押したか
	bool isAttackDecision = false;  // 攻撃決定ボタンを押したか
	bool isDefenseDecision = false; // 防御決定ボタンを押したか
	bool isHealAction = false;		// 回復系カードであることを示すフラグ
	// 今後「選んだカードのID」などもここに追加していくと、通信がさらに楽になります
};

class BattleInputManager
{
public:
	// 更新処理
	PlayerAction Update(BattleData& data,
		const InputManager& input,
		Player& humanPlayer,
		int humanIdx,
		bool isHumanTurn);

private:
	// 降参（サレンダー）関連のUI処理
	void ProcessSurrender(BattleData& data, const InputManager& input, PlayerAction& action);

	// 攻撃・防御の決定ボタン処理
	void ProcessActionButtons(BattleData& data, const InputManager& input, int humanIdx, bool isHumanTurn, PlayerAction& action);

	// マニュアルでのターゲット選択処理（ステータスUIクリック）
	void ProcessTargetSelection(BattleData& data, const InputManager& input, bool isHumanTurn);

	// 手札のホバー・クリック（選択とコンボ）処理
	void ProcessHandSelection(BattleData& data, const InputManager& input, Player& humanPlayer, int humanIdx, bool isHumanTurn);
};

