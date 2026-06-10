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

public:
    TrainingLobbyScene();
    virtual SceneName Update(const InputManager& input) override;
};