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
DamageResult DamageResolver::CalculateDamage(const TotalAttack& attack, const TotalDefense& defense) {
    DamageResult result;
    result.isHit = true;
    result.isInstantDeath = false;

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

    // --- 防御判定（TotalDefenseを使用） ---
    if (defense.isActive) {
        // 合計された属性同士で相性チェック
        if (IsValidGuard(attack.type, defense.type)) {
            // 防御成立：合計防御力を引く
            // result.isGuardSuccess = true; // 構造体にフラグがある場合
            result.finalDamage -= defense.power;
        }
        else {
            // 防御不成立（相性不良など）
            // result.isGuardSuccess = false;
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