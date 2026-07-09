#pragma once
#include "InputManager.h"
#include "Player.h"

struct BattleData;
struct LocalClientData;
class InputManager;
class Player;

// プレイヤーの「行動」をまとめる構造体
struct PlayerAction {
	bool hasAction = false;         // 何かしらの行動を決定したか
	bool isSurrender = false;       // 降参ボタンを押したか
	bool isAttackDecision = false;  // 攻撃決定ボタンを押したか
	bool isDefenseDecision = false; // 防御決定ボタンを押したか
	bool isHealAction = false;		// 回復系カードであることを示すフラグ

    // --- 【追加】通信でホストに具体的な行動を伝えるための変数 ---
    std::vector<int> selectedCardIdxs;       // 選んだカードのインデックス（コンボ対応）
    int targetIdx = -1;             // 攻撃(または回復)の対象となるプレイヤーのインデックス


	// 今後「選んだカードのID」などもここに追加していくと、通信がさらに楽になります
};

class BattleInputManager
{
public:
    // 更新処理
    PlayerAction Update(BattleData& data,
        LocalClientData& localData,
        const InputManager& input,
        Player& humanPlayer,
        int humanIdx,
        bool isHumanTurn);

private:
    // 降参（サレンダー）関連のUI処理
    void ProcessSurrender(BattleData& data, LocalClientData& localData, const InputManager& input, PlayerAction& action);

    // 攻撃・防御の決定ボタン処理
    void ProcessActionButtons(BattleData& data, LocalClientData& localData, const InputManager& input, int humanIdx, bool isHumanTurn, PlayerAction& action);

    // マニュアルでのターゲット選択処理
    void ProcessTargetSelection(BattleData& data, LocalClientData& localData, const InputManager& input, bool isHumanTurn);

    // 手札のホバー・クリック（選択とコンボ）処理
    void ProcessHandSelection(BattleData& data, LocalClientData& localData, const InputManager& input, Player& humanPlayer, int humanIdx, bool isHumanTurn);
};

