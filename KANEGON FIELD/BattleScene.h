// 戦闘(ゲームプレイ)画面クラス
#pragma once
#include "InterfaceScene.h"
#include "BattleData.h"
#include "BattleInputManager.h"
#include "BattleLogicManager.h"
#include "BattleAIManager.h"
#include "BattleUIManager.h"
#include "NetworkManager.h"

class BattleScene :public IScene
{
private:
	NetworkManager* netManager = nullptr; // 通信管理へのポインタ

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

	// 通信マネージャーをセットする関数を追加
	void SetNetworkManager(NetworkManager* manager) { netManager = manager; }

	// プレイヤーを受け取る初期化関数を追加
	void Initialize(const std::vector<Player>& initialPlayers);

	// ISceneの仮想関数をオーバーライド
	SceneName Update(const InputManager& input) override;
	void Draw() const override;
};