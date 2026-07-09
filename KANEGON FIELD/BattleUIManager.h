#pragma once
#include <vector>
#include <string>
#include "Player.h"
#include "Card.h"
#include "BattleData.h"

// 座標計算用構造体
struct Rect {
    int x, y, w, h;

    // メソッド

    // 点がこの矩形内にあるか判定
    bool Contains(int px, int py) const {
        return (px >= x && px <= x + w && py >= y && py <= y + h);
    }

    // 矩形を移動する
    void Offset(int dx, int dy) {
        x += dx;
        y += dy;
    }
};

// クラス定義（前方宣言などが必要な場合は適宜追加してください）
struct LocalClientData;

class BattleUIManager
{
public:
    // 全体の描画を管理する窓口
    void Draw(const BattleData& data, const LocalClientData& local) const;

private:

    // 属性の色処理関数
    unsigned int GetElementColor(const std::string& elementType) const;

    // プレイヤーステータスの描画
    void DrawPlayerStatus(const BattleData& data, const LocalClientData& local) const;

    // 手札の描画
    void DrawPlayerHand(const BattleData& data, const Player& player, const LocalClientData& local) const;

    // 選択されたカードを描画する関数
    void DrawSelectedCard(const BattleData& data, const Player& player, const LocalClientData& local) const;

    // BattleInput がクリック判定に使うための関数
    Rect GetHandCardRect(int handIndex) const;

    // 今のターンのプレイヤー名表示
    void DrawTurnPlayerName(const Player& player) const;

    // ターゲット指定されたプレイヤー名表示
    void DrawTargetPlayerName(const BattleData& data, const LocalClientData& local) const;

    // 防御側のカード表示
    void DrawDefenseCards(const BattleData& data, const Player& player, const LocalClientData& local) const;

    // 降参処理表示用関数
    void DrawSurrenderWindow(const LocalClientData& local) const;

    // カード選択決定ボタンの描画(名前の下に薄い四角を表示するだけ)
    void DrawCardSelectButton(const LocalClientData& local) const;

    // リザルト画面
    void DrawEndScreen(const BattleData& data, const LocalClientData& local) const;

    // ダメージ・回復のポップアップ描画
    void DrawPopups(const BattleData& data, const LocalClientData& local) const;

    // 換（ステータス変更）ウィンドウの描画
    void DrawChangeStatusWindow(const BattleData& data, const LocalClientData& local) const;

    // 買（購入確認）ウィンドウの描画
    void DrawBuyConfirmWindow(const BattleData& data, const LocalClientData& local) const;
};