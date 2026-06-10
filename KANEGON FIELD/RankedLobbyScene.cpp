#include "RankedLobbyScene.h"
#include "Picture.h"
#include "DxLib.h"

extern Player g_player;

RankedLobbyScene::RankedLobbyScene()
    : BaseLobbyScene(SelectScene::Option::TAIMAN) // TAIMANモードとして初期化
{
    // 必要であればここでランキングデータなどを読み込む
}

SceneName RankedLobbyScene::Update(const InputManager& input) {
    // 共通のホバー判定リセット
    for (int i = 0; i < MAX; i++) { isHoverIdx[i] = false; }

    // 戻るボタンとスタートボタンのホバー判定
    isHoverIdx[RETURN] = input.IsMouseOver(RET_BUT_X, RET_BUT_Y, RET_BUTW, RET_BUTH);
    isHoverIdx[BATTLE_START] = input.IsMouseOver(350, 550, 300, 150);

    // ランキングボタンのホバー判定（例）
    isHoverIdx[RANKING] = input.IsMouseOver(50, 100, 200, 100);

    if (input.IsLeftClicked()) {
        if (isHoverIdx[RETURN]) return SceneName::SELECT;
        if (isHoverIdx[BATTLE_START]) {
            // 真剣勝負のプレイヤー設定（自分のみ追加など）
            BattlePlayer.clear();
            Player user;
            user.setName(g_player.getName());
            user.setControllerType(ControllerType::HUMAN);
            BattlePlayer.push_back(user);

            return SceneName::BATTLE;
        }
    }
    return SceneName::SETTING;
}

void RankedLobbyScene::DrawSpecificUI() const {
    // ランキングボタンの描画
    unsigned int rankColor = isHoverIdx[RANKING] ? Col.GetCurYel() : Col.GetWhi();
    DrawBox(50, 100, 250, 200, Col.GetBla(), FALSE);
    DrawBox(50, 100, 250, 200, rankColor, TRUE);
    DrawString(70, 130, _T("ランキング"), Col.GetBla());

    // ここに現在ランクなどの情報を描画
    DrawFormatStringToHandle(300, 100, Col.GetBla(), Font.GetBig(), _T("近日公開\nランキングシステム"));
}