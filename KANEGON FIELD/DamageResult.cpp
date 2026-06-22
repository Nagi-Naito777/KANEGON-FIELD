#include "DamageResult.h"

// 防御が成立するかどうかを判定するヘルパー関数
bool DamageResolver::IsValidGuard(const std::string& atkAttr, const std::string& defAttr) {
    // 光属性防御はどの属性攻撃も守れる（最強の盾）
    if (defAttr == "光") return true;

    // 闇属性の攻撃判定
    // 「盾は機能する（ダメージを減らせる）」が、ダメージが残れば即死する仕様
    if (atkAttr == "闇") return true;

    // 3すくみ (炎 -> 水, 水 -> 木, 木 -> 炎)
    if (atkAttr == "炎" && defAttr == "水") return true;
    if (atkAttr == "水" && defAttr == "木") return true;
    if (atkAttr == "木" && defAttr == "炎") return true;

    // 光属性の攻撃判定
    // 光の攻撃は「光の盾」以外では防げない（上記1で判定済みのため、ここでは弾かれる）
    if (atkAttr == "光") return false;

    // 無属性・その他
    if (atkAttr == "無" || atkAttr == "") return true;

    // 上記以外は防げない
    return false;
}

// 計算結果だけを算出する関数
DamageResult DamageResolver::CalculateDamage(const TotalAttack& attack, const TotalDefense& defense) {
    DamageResult result;
    result.isHit = true;
    result.isInstantDeath = false;
    result.isGuardSuccess = false;
    result.finalDamage = attack.power;

    // 全体攻撃(All)の命中判定
    if (attack.isAll) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(1, 100);

        if (dist(gen) > attack.hitPercent) {
            result.isHit = false; // Miss!
            result.finalDamage = 0;
            return result;
        }
    }

    // 防御判定
    if (defense.isActive) {
        if (IsValidGuard(attack.type, defense.type)) {
            result.isGuardSuccess = true;
            result.finalDamage -= defense.power;
        }
    }

    // ダメージの下限処理（マイナスにはならない）
    result.finalDamage = (std::max)(0, result.finalDamage);

    // 闇属性の特殊ルール（即死判定）
    // 防御を抜けて1ダメージでも食らったら即死
    if (attack.type == "闇" && result.finalDamage > 0) {
        result.isInstantDeath = true;
    }

    return result;
}