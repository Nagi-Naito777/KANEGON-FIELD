// 戦闘(ゲームプレイ)画面クラス
#pragma once
#include "InterfaceScene.h"
#include "BattleData.h"
#include "BattleInputManager.h"
#include "BattleLogicManager.h"
#include "BattleAIManager.h"
#include "BattleUIManager.h"

class BattleScene :public IScene
{
private:
	BattleData data; // 全てのデータ変数等の格納場所

	// マネージャーたち
	BattleInputManager inputManager;
	BattleLogicManager logicManager;
	BattleAIManager aiManager;
	BattleUIManager uiManager;

public:
	// コンストラクタとデストラクタ
	BattleScene();
	~BattleScene()override;

	// ISceneの仮想関数をオーバーライド
	SceneName Update(const InputManager& input) override;
	void Draw() const override;
};