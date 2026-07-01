// 乱闘モード用クラス
#pragma once
#include "BaseLobbyScene.h"
#include "NetworkManager.h"

class BrawlLobbyScene : public BaseLobbyScene
{
private:
    bool isTeamState[MAX]; // SettingSceneのisTeamを改名（競合防止）
    bool isBattlePlayer[MEMBER_MAX];

    void SelectTeam(int teamId);

    NetworkManager* netManager = nullptr;
    bool isConnected = false;       // 接続済みかどうか
    bool isWaitingStart = false;    // ホストの開始を待っている状態

protected:
    virtual void DrawSpecificUI() const override;

public:

    void SetNetworkManager(NetworkManager* manager) { netManager = manager; }

    BrawlLobbyScene();
    virtual SceneName Update(const InputManager& input) override;
};