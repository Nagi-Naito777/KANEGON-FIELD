#pragma once
#include <string>
#include <random>
#include "Card.h"
#include "Player.h"

// --- データの定義 ---

struct TotalAttack {
    int power = 0;          // 合計威力
    std::string type = "";  // 属性（炎、水など）
    int hitPercent = 100;   // 命中率（全体攻撃などの場合）
    bool isAll = false;     // 全体攻撃フラグ
};

struct DamageResult {
    int finalDamage = 0;         // 最終的に受けるダメージ量
    bool isHit = true;           // 命中したかどうか（Missならfalse）
    bool isInstantDeath = false; // 闇属性の即死が発動したか
    bool isGuardSuccess = false; // 防御が成立したか（相性で防げたか）
    int statusEffectID = 0;      // 状態異常ID（0ならなし）
};


// --- ダメージ計算機 ---

class DamageResolver {
public:
    // 防御が成立するかどうかを判定するヘルパー関数
    static bool IsValidGuard(const std::string& atkAttr, const std::string& defAttr);

    // 計算結果だけを算出する関数
    static DamageResult CalculateDamage(const TotalAttack& attack, const Card* defenseCard = nullptr);
};