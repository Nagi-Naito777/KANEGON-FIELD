// 戦闘(ゲームプレイ)画面クラス
#pragma once
#include "InterfaceScene.h"
#include "BattleData.h"
#include "BattleInputManager.h"
#include "BattleLogicManager.h"
#include "BattleAIManager.h"
#include "BattleUIManager.h"
#include "NetworkManager.h"
#include <vector>

class BattleScene : public IScene
{
private:
    // --- メンバ変数 ---
    NetworkManager* netManager = nullptr; // 通信管理へのポインタ

    BattleData data;           // 戦闘中の全プレイヤーデータ（共有）
    LocalClientData localData; // 自身の操作用データ（ローカル）

    // マネージャーたち
    BattleInputManager inputManager;
    BattleLogicManager logicManager;
    BattleAIManager aiManager;
    BattleUIManager uiManager;

    // クライアントのHP低下（ダメージ）検知用
    std::vector<int> prevHps;

    // --- 【初期化・準備系】 ---

    // AIの生成、名前の割り当て（オフライン対戦時のみ）
    void SetupOfflineAI();

    // カードの初期配布処理（ホストが全員分を配る）
    void DistributeInitialCards(bool isOnline);

    // プレイヤーのID（自分を何番目とするか）の決定
    void AssignPlayerIDs(bool isOnline);

    // --- 【通信処理系】 ---

    // 受信したパケットを種類ごとに振り分ける「窓口」
    void HandleNetworkReceive();

    // [ホスト用] クライアントから届いた操作（攻撃/防御など）を処理する
    void ProcessClientAction(const GamePacket& packet);

    // [クライアント用] ホストから届いた最新のゲーム状況を反映する
    void ProcessHostSyncData(const GamePacket& packet);

    // --- 【ゲームループ・ロジック系】 ---

    // HP減少などを検知して、エフェクトやUIイベントを発火させる
    void UpdateClientUIEvents();

    // プレイヤーのアクション（攻撃/防御の決定）をゲームに反映・送信する
    void ProcessPlayerAction(const PlayerAction& action, bool isOnline);

    // [ホスト用] 全体の進行管理、AI処理、クライアントへの同期送信を一括で行う
    void UpdateHostLogicAndSync();

public:
    BattleScene();
    ~BattleScene() override;

    // 通信マネージャーのセットアップ
    void SetNetworkManager(NetworkManager* manager) { netManager = manager; }

    // 戦闘開始時の初期化
    void Initialize(const std::vector<Player>& initialPlayers);

    // 毎フレームの更新処理（ISceneインターフェース）
    SceneName Update(const InputManager& input) override;

    // 描画処理（ISceneインターフェース）
    void Draw() const override;
};