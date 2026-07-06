// 修行モード用クラス
#pragma once
#include "BaseLobbyScene.h"

class TrainingLobbyScene : public BaseLobbyScene
{
private:
    bool MemberCustom;
    int selectedMemberCount;
    bool isHoverIdx2[9];

protected:
    virtual void DrawSpecificUI() const override;

    // 修行モードでは常にスタートボタンを表示する
    virtual bool ShouldDrawStartButton() const override { return true; }
public:
    virtual std::vector<Player> GetBattlePlayers() const override;

    TrainingLobbyScene();
    virtual SceneName Update(const InputManager& input) override;
};