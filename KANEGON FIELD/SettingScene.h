// バトル設定用画面クラス
#pragma once

#include "DxLib.h"
#include <string>
#include <vector>
#include "InterfaceScene.h"
#include "SelectScene.h"
#include "GameConfig.h"

class Player;
class InputManager;

class SettingScene :public IScene
{
    // 詳細設定列挙体
    enum BattleOption {
        NONE = -1,      // 何も選択されてない
        BATTLE_START,   // バトル開始
        MEMBER,         // 修行と乱闘用の人数変更
        PVP,            // 個人戦参加(乱闘)
        TEAM_RED,       // チームレッドで参加(乱闘)
        TEAM_BLUE,      // チームブルーで参加(乱闘)
        TEAM_YELLOW,    // チームイエローで参加(乱闘)
        TEAM_GREEN,     // チームグリーンで参加(乱闘)
        RANKING,        // ランキングUI表示(真剣勝負用)
        RETURN,         // 一個前の画面に戻る
        MAX             // 詳細設定選択最大数
    };

public:
    // コンストラクタ：セレクト画面で選ばれたモードを受け取る
    SettingScene(SelectScene::Option mode);
    ~SettingScene() override = default;

    // 必須のオーバーライド関数
    SceneName Update(const InputManager& input) override;
    void Draw() const override;

    // バトルに参加するプレイヤーリストを外部(BattleScene等)へ渡す関数
    const std::vector<Player>& GetBattlePlayers() const { return BattlePlayer; }

private:
    // --- 内部処理用のヘルパー関数  ---
    void DrawPlayerTeam(const std::string& nameStr, int y, unsigned int bgColor) const;
    void BlackDrawBox(int x, int y, int x2, int y2) const;
    void SelectTeam(int teamId);

    SelectScene::Option currentMode;    // 前の画面で選ばれたモードを保持

    std::vector<Player> BattlePlayer;   // 対戦に参加してる人数
    int selectedOption;                 // 現在選ばれている選択肢

    bool isHoverIdx[MAX];               // 各ボタンの上にマウスがあるか
    bool isHoverIdx2[9];                // 人数選択時のマウスの判定枠
    bool isTeam[MAX];                   // その対戦形式になってるかの有無を格納する変数
    bool isBattlePlayer[MEMBER_MAX];    // 最大対戦人数分の参加の有無を判定する配列

    bool MemberCustom;                  // 対戦人数変更ウィンドウがオンになってるか
    int selectedMemberCount;            // 選択したメンバーの数を格納する変数(初期値は2)
};

