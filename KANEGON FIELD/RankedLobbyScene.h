// 真剣勝負モード用クラス
#include "BaseLobbyScene.h"

class RankedLobbyScene : public BaseLobbyScene
{
protected:
    // 真剣勝負専用の描画処理
    virtual void DrawSpecificUI() const override;

public:
    virtual std::vector<Player> GetBattlePlayers() const override;

    RankedLobbyScene();
    virtual SceneName Update(const InputManager& input) override;
};