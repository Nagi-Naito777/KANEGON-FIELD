#pragma once
#include <string>
#include "DxLib.h"
#include "PlayerHand.h"

// AIかプレイヤーか判断する列挙体
enum class ControllerType {
    HUMAN,      // 自分のPCで操作する人
    AI,         // AI
    NETWORK     // オンライン相手
};

// 状態異常フラグをまとめた構造体（分かりやすくひとまとめにする）
struct StatusEffects {
    bool dead = false;      // 気絶状態
    bool poison = false;    // 毒状態
    bool mist = false;      // 霧
    bool rock = false;      // ランダム1枚使用不可
    bool flash = false;     // 防御カード1枚制限
    bool darkness = false;  // 確率攻撃確定ヒット
};

// プレイヤークラス
class Player {
private:
    int ID;                     // ユーザー識別の個別ID(後にオンライン化するために必須)
    std::string name;           // ユーザーネーム
    ControllerType controlType; // AIかどうかの判定
    int team;                   // どこのチームか決める変数

    int hp;                     // HP
    int mp;                     // MP
    int money;                  // お金

public:
    PlayerHand Hand;            // 手札管理パーツ
    StatusEffects Status;       // 状態異常パーツ

    // コンストラクタ
    Player();

    // --- Getter ---
    ControllerType getControllerType() const { return controlType; }
    std::string getName() const { return name; }
    int getHp() const { return hp; }
    int getMp() const { return mp; }
    int getMoney() const { return money; }

    // --- Setter ---
    void setControllerType(ControllerType type) { controlType = type; }
    void setName(const std::string& newName) { name = newName; }

    // 値の制限（0～99）を行うセッター
    void setHp(int newHp);
    void setMp(int newMp);
    void setMoney(int newMoney);
};

extern Player g_player;