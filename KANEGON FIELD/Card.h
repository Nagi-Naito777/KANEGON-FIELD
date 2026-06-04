// カードクラス
#pragma once
#include <string>

// カードの種類を定義する列挙体
enum CardCategory {
    Change,         // "換"　ステータス変換
    Sell,           // "売"　売る
    Buy,            // "買"　買う
    Attack,         // "攻"　攻撃カード
    Bilingual,      // "両"　攻撃・防御どちらも可能
    All,            // "全"　全体に確率攻撃
    Defense,        // "守"　防御カード
    Healing,        // "癒"　HP回復カード
    MagicHealing,   // "魔"　MP回復カード
    Magic,          // "奇"　奇跡(呪文)カード
    UnNull          // "無"　どこにも属さない
};

class Card {
private:
    // カードデータを構造体としてまとめる（Date -> Dataに修正）
    struct CardData {
        int ID = 0;
        std::string name;
        int power = 0;
        std::string type;
        std::string setumei;
        CardCategory category = UnNull;
        bool add = false;
        int money = 0;
        int MP = 0;
        int percent = 100;
    };

    CardData data;

public:
    int graphicIndex; // Pic.Card[graphicIndex] に対応する番号

    // デフォルトコンストラクタ
    Card();

    // 値をセットするコンストラクタ（カテゴリは変換済みのものを受け取る）
    Card(int id, std::string name, int power, std::string type,
        std::string setumei, CardCategory category, bool can_add, int money, int mp, int percent);

    // ゲッター関数
    int GetID() const { return data.ID; }
    int GetPower() const { return data.power; }
    int GetMoney() const { return data.money; }
    int GetMP() const { return data.MP; }
    CardCategory GetCategory() const { return data.category; }
    int GetPercent() const { return data.percent; }

    const std::string& GetName() const { return data.name; }
    const std::string& GetType() const { return data.type; }
    const std::string& GetDescription() const { return data.setumei; }

    bool GetAdd() const { return data.add; }
};

