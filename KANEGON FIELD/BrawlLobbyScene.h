// 乱闘モード用クラス
#pragma once
#include "BaseLobbyScene.h"
#include "NetworkManager.h"
#include <vector>
#include <string>

// ロビーにいるプレイヤーの情報を管理する構造体
struct LobbyPlayer {
    std::string name; // プレイヤー名
    int teamId;        // 選択したチーム
};

class BrawlLobbyScene : public BaseLobbyScene
{
private:
    bool isTeamState[MAX]; // SettingSceneのisTeamを改名（競合防止）
    bool isBattlePlayer[MEMBER_MAX];

    size_t m_prevClientCount = 0; // 前回までのクライアント接続数

    void SelectTeam(int teamId);

    NetworkManager* netManager = nullptr;
    bool isConnected = false;       // 接続済みかどうか
    bool isWaitingStart = false;    // ホストの開始を待っている状態

    // マルチプレイ同期用の変数と関数
    bool isHost = false;                         // 自分がホストかどうかを記憶
    std::vector<LobbyPlayer> m_lobbyPlayers;     // 参加順に並ぶプレイヤーリスト

    // バトル開始条件を満たしているかチェックする関数
    bool CanStartBattle() const;

protected:
    virtual void DrawSpecificUI() const override;
protected:
    virtual SceneName OnReturnClicked() override;
    virtual bool ShouldDrawStartButton() const override;

public:
    virtual std::vector<Player> GetBattlePlayers() const override;

    void SetNetworkManager(NetworkManager* manager) override { netManager = manager; }

    BrawlLobbyScene();
    virtual SceneName Update(const InputManager& input) override;
};