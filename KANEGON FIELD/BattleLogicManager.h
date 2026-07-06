#pragma once
#include "BattleData.h"
#include "Player.h"
#include "DamageResult.h"

class BattleLogicManager {
public:
    // 毎フレーム呼ばれるロジックの更新処理（両方のデータが必要）
    void Update(BattleData& data, LocalClientData& local);

    // 攻撃属性の再計算（カード選択時に UI/Input から呼ばれる想定）
    void RecalculateAttackElement(BattleData& data, LocalClientData& local, const std::vector<Card>& hand);

    // 防御属性の更新処理
    void RecalculateDefenseElement(BattleData& data, LocalClientData& local, const std::vector<Card>& hand);

    // カードの説明文を返す関数（データ不変のため変更なし）
    static std::string GetCardEffectDescription(const Card& card);

    // 実際の効果発動関数(純粋なロジック処理のため BattleData のみ)
    void ExecuteCardEffect(BattleData& data, LocalClientData& local, Player& attacker, Player* target, const Card& card);

    // MPが足りているかチェックする関数 (純粋なロジックのため Player のみ)
    bool CanUseMiracleCard(const Player& player, const Card& card);

    // 攻撃・防御で共通して使う属性の合成ロジック
    std::string GetCombinedElement(const std::vector<int>& selectedIdxs, const std::vector<Card>& hand);

    // カードの選択可否を更新する関数（LocalClientData に結果を書き込むため必要）
    void UpdateCardSelectability(const BattleData& data, LocalClientData& local, const Player& player);

    // UI側で防御カードを選択できるか判定する関数
    bool CanSelectDefenseCard(const BattleData& data, const std::vector<int>& currentSelectedCards,
        const Player& defender, int cardIdx, const std::string& incomingAttackElement);

private:
    // フェーズごとの処理（UIフラグや入力状態を触るため、すべて local が必要）
    void ProcessSelectPhase(BattleData& data, LocalClientData& local);
    void ProcessDefenseSelectPhase(BattleData& data, LocalClientData& local);
    void ProcessAttackRevealPhase(BattleData& data, LocalClientData& local);
    void ProcessTargetDisplayPhase(BattleData& data, LocalClientData& local);
    void ProcessDefenseRevealPhase(BattleData& data, LocalClientData& local);
    void ProcessEffectPhase(BattleData& data, LocalClientData& local);
    void ProcessDamageResultPhase(BattleData& data, LocalClientData& local);
    void ProcessIdlePhase(BattleData& data, LocalClientData& local);
    void ProcessChangeStatusEditPhase(BattleData& data, LocalClientData& local);
    void ProcessBuyConfirmPhase(BattleData& data, LocalClientData& local);

    // --- 内部で処理するための補助関数 ---

    // ターンを次に進める（UIのリセットも兼ねるため local を追加）
    void NextTurn(BattleData& data, LocalClientData& local);

    // プレイヤーの脱落処理（dataを書き換えるため const を外す）
    void RemovePlayer(BattleData& data, int targetIdx);

    // 合計威力の計算（データ参照のみのため const）
    TotalAttack CalculateTotalAttack(const BattleData& data, Player& attacker);

    // ダメージ判定と適用（UI演出用に local を追加）
    void ResolveDamage(BattleData& data, LocalClientData& local, Player& attacker, Player& target, const TotalAttack& attack, const TotalDefense& defense);

    // 合計防御威力の計算（データ参照のみのため const）
    TotalDefense CalculateTotalDefense(const BattleData& data, Player& defender);

    // アニメーション処理（ローカルデータの更新が主）
    void UpdateCardAnimation(BattleData& data, LocalClientData& local);

    // 回復系カード用の判定枠ヘルパー関数
    bool IsHealingAction(const BattleData& data, const Player& attacker);

    // 自分自身が選択された時の判定関数
    bool IsSelfTarget(const BattleData& data, const LocalClientData& local);
};