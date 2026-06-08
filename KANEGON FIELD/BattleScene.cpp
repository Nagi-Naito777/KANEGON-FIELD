#include "BattleScene.h"

// 戦闘ロジックの中身を見やすくするために分割したヘッダー
#include "BattleAIManager.h"
#include "BattleUIManager.h"
#include "BattleInputManager.h"
#include "BattleLogicManager.h"
#include "BattleData.h"

SceneName BattleScene::Update(const InputManager& input) {
	// =============================================================
	 // 操作プレイヤー（人間）の情報を検索・特定する
	 // =============================================================
	int humanIdx = 0;

	for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
		// 先ほど作った ControllerType::HUMAN で人間プレイヤーを探す
		if (data.Player_Turn[i].getControllerType() == ControllerType::HUMAN) {
			humanIdx = i;
			break;
		}
	}

	// 抽出した情報を変数にまとめる
	Player& humanPlayer = data.Player_Turn[humanIdx];
	bool isHumanTurn = (data.currentTurnIdx == humanIdx);

	// =============================================================
	// マネージャーの更新処理
	// =============================================================

	// 入力をデータに反映（先ほど作った引数をすべて渡す）
	bool isSurrender = inputManager.Update(data, input, humanPlayer, humanIdx, isHumanTurn);

	// もし降参（GIVE_UP）が選択されたら、戦闘シーンを終了する
	if (isSurrender) {
		// ※遷移先のシーン名はご自身のゲームに合わせて変更してください（Title, Resultなど）
		// return SceneName::Result; 
	}

	// AIの思考をデータに反映
	// aiManager.Update(data);

	// ルールに従ってデータを更新（ダメージ計算など）
	// logicManager.Update(data);

	// =============================================================
	// シーンの継続
	// =============================================================
	return SceneName::BATTLE; // 通常時はそのまま戦闘シーンを続ける
}

void BattleScene::Draw() const {
    // 描画はデータを見てUIマネージャーに任せる
    //uiManager.Draw(data);
}