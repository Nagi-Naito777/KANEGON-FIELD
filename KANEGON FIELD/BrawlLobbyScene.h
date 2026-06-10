// 乱闘モード用クラス
#pragma once
#include "BaseLobbyScene.h"

class BrawlLobbyScene : public BaseLobbyScene
{
private:
    bool isTeamState[MAX]; // SettingSceneのisTeamを改名（競合防止）
    bool isBattlePlayer[MEMBER_MAX];

    void SelectTeam(int teamId);

protected:
    virtual void DrawSpecificUI() const override;

public:
    BrawlLobbyScene();
    virtual SceneName Update(const InputManager& input) override;
};