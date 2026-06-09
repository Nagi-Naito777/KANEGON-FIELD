#pragma once
#include "BattleData.h"
#include "Player.h"
#include "DamageResult.h"

class BattleLogicManager {
public:
    // 毎フレーム呼ばれるロジックの更新処理
    void Update(BattleData& data);

    // 攻撃属性の再計算（カード選択時に UI/Input から呼ばれる想定）
    void RecalculateAttackElement(BattleData& data, const std::vector<Card>& hand);

private:
    // --- 内部で処理するための補助関数 ---
    // ターンを次に進める
    void NextTurn(BattleData& data);

    // プレイヤーの脱落処理
    void RemovePlayer(BattleData& data, int targetIdx);

    // 合計威力の計算
    TotalAttack CalculateTotalAttack(BattleData& data, Player& attacker);

    // ダメージ判定と適用
    void ResolveDamage(BattleData& data, Player& target, const TotalAttack& attack, const Card* defenseCard);
};