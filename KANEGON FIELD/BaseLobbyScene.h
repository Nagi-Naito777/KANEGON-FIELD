// 対戦前の設定用画面の基盤クラス
#pragma once
#include "InterfaceScene.h"
#include "InputManager.h"
#include "GameConfig.h"
#include "Player.h"
#include "SelectScene.h"
#include <string>
#include <vector>
class BaseLobbyScene : public IScene
{
protected:
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
        BTN_HOST,       // ホストになるボタン
        BTN_CLIENT,     // クライアントになるボタン
        MAX             // 詳細設定選択最大数
    };

    SelectScene::Option currentMode;
    bool isHoverIdx[MAX];
    std::vector<Player> BattlePlayer; // 対戦に参加するプレイヤーリスト

    // 共通の描画ヘルパー関数
    void DrawPlayerTeam(const std::string& nameStr, int y, unsigned int bgColor) const;
    void BlackDrawBox(int x, int y, int x2, int y2) const;

    // 子クラスごとに固有のUIを描画するための仮想関数
    virtual void DrawSpecificUI() const = 0;

public:
    BaseLobbyScene(SelectScene::Option mode);
    virtual ~BaseLobbyScene() override = default;

    // 全モード共通のDraw処理（Template Method パターン）
    void Draw() const override;

    // 外部へプレイヤーリストを渡す関数
    const std::vector<Player>& GetBattlePlayers() const { return BattlePlayer; }

    // Updateは子クラスに強制実装させる
    virtual SceneName Update(const InputManager& input) = 0;

    // 各シーンのクラス定義に追加
    void SetNetworkManager(NetworkManager* net) override {} // 何もしなくてOK
};

