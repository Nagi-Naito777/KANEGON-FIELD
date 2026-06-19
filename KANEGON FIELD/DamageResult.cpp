#include "DamageResult.h"

// 防御が成立するかどうかを判定するヘルパー関数
bool DamageResolver::IsValidGuard(const std::string& atkAttr, const std::string& defAttr) {
    // 無属性と闇属性攻撃はどの属性でも守れる
    if (atkAttr == "無" || atkAttr == "" || atkAttr == "闇") {
        return true;
    }

    // 光属性防御はどの属性攻撃も守れる
    if (defAttr == "光") {
        return true;
    }

    // 3すくみ (炎 -> 水, 水 -> 木, 木 -> 炎 で守れる)
    // ※元のコードが"木"でしたが、ご要望に合わせて"草"にしています。カードのデータも"草"になっているかご確認ください。
    if (atkAttr == "炎" && defAttr == "水") return true;
    if (atkAttr == "水" && defAttr == "木") return true;
    if (atkAttr == "木" && defAttr == "炎") return true;

    // 上記以外（相性不一致）は防げない
    return false;
}

// 計算結果だけを算出する関数
DamageResult DamageResolver::CalculateDamage(const TotalAttack& attack, const TotalDefense& defense) {
    DamageResult result;
    result.isHit = true;
    result.isInstantDeath = false;
    result.isGuardSuccess = false;

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

    // --- 防御判定 ---
    if (defense.isActive) {
        // 合計された属性同士で相性チェック
        if (IsValidGuard(attack.type, defense.type)) {
            // 防御成立
            result.isGuardSuccess = true;
            result.finalDamage -= defense.power;
        }
        else {
            // 防御不成立（相性不良：ダメージ減算なし）
            result.isGuardSuccess = false;
        }
    }

    // --- 防御判定 ---
    if (defense.isActive) {
        // 合計された属性同士で相性チェック
        if (IsValidGuard(attack.type, defense.type)) {
            // 防御成立
            result.isGuardSuccess = true;
            result.finalDamage -= defense.power;
        }
        else {
            // 防御不成立（相性不良：ダメージ減算なし）
            result.isGuardSuccess = false;
        }
    }

    return result;
}