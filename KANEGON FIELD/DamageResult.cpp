#include "DamageResult.h"

// 防御が成立するかどうかを判定するヘルパー関数
bool DamageResolver::IsValidGuard(const std::string& atkAttr, const std::string& defAttr) {
    if (defAttr == "光") return true;

    if (atkAttr == "闇") {
        return true;
    }

    if (atkAttr == "炎") return (defAttr == "水");
    if (atkAttr == "水") return (defAttr == "木");
    if (atkAttr == "木") return (defAttr == "炎");

    if (atkAttr == "光") return false;

    if (atkAttr == "無" || atkAttr == "") return true;

    return false;
}

// 計算結果だけを算出する関数
DamageResult DamageResolver::CalculateDamage(const TotalAttack& attack, const Card* defenseCard) {
    DamageResult result;

    // 全体攻撃(All)の命中判定
    if (attack.isAll) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(1, 100);

        if (dist(gen) > attack.hitPercent) {
            result.isHit = false; // Miss!
            return result;        // これ以上計算せず返す
        }
    }

    // 基本ダメージ量
    result.finalDamage = attack.power;

    // 防御カードの適用
    if (defenseCard != nullptr) {
        if (IsValidGuard(attack.type, defenseCard->GetType())) {
            // 防御成立
            result.isGuardSuccess = true;
            result.finalDamage -= defenseCard->GetPower();
        }
        else {
            // 防御不成立（相性悪や、光属性に他属性でガードした場合）
            result.isGuardSuccess = false;
        }
    }

    // ダメージがマイナスにならないよう下限を0にする
    if (result.finalDamage < 0) result.finalDamage = 0;

    // 闇属性の特殊ルール（即死判定）
    if (attack.type == "闇" && result.finalDamage > 0) {
        // 防御しきれず1ダメージでも食らったら即死フラグを立てる
        result.isInstantDeath = true;
    }

    return result;
}