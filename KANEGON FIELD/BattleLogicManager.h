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
    void ResolveDamage(BattleData& data, Player& target, const TotalAttack& attack, const TotalDefense& defense);

    // 合計防御威力の計算
    TotalDefense CalculateTotalDefense(BattleData& data, Player& defender);

    // 防御属性の更新処理
    void RecalculateDefenseElement(BattleData& data, const std::vector<Card>& hand);

    // アニメーション処理
    void UpdateCardAnimation(BattleData& data);

    // 攻撃・防御で共通して使う属性の合成ロジック
    std::string GetCombinedElement(const std::vector<int>& selectedIdxs, const std::vector<Card>& hand);

    // UI側で防御カードを選択できるか判定する関数
    bool CanSelectDefenseCard(const BattleData& data, const Player& defender, int cardIdx, const std::string& incomingAttackElement);
};