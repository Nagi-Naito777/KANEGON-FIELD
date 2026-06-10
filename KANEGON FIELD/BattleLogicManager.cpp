#include "BattleLogicManager.h"
#include "CardDatabase.h"
#include <algorithm> // ソート用

void BattleLogicManager::Update(BattleData& data) {
    // =============================================================
    // Reveal（公開演出）フェーズの進行
    // =============================================================
    if (data.currentPhase == BattlePhase::Reveal) {
        if (data.animationTimer > 0) {
            data.animationTimer--;
        }
        else {
            if (data.revealIndex < data.selectedCards.size()) {
                data.revealIndex++;
                data.animationTimer = 30; // 次のカードを開くまでの時間
            }
            else {
                // すべて公開し終わったらエフェクトフェーズへ
                data.currentPhase = BattlePhase::Effect;
                data.animationTimer = 60;
            }
        }
        return; // 自動進行フェーズ中はここで処理を終える
    }

    // =============================================================
    // Effect / Damage（ダメージ計算・削除・ドロー）フェーズの進行
    // =============================================================
    if (data.currentPhase == BattlePhase::Effect || data.currentPhase == BattlePhase::Damage) {
        if (data.animationTimer > 0) {
            data.animationTimer--;
        }

        if (data.animationTimer == 0) {
            if (data.currentPhase == BattlePhase::Effect) {
                // --- ダメージ計算の実行 ---
                Player& attacker = data.Player_Turn[data.currentTurnIdx];
                Player* target = nullptr; // ポインタで保持する

                if (data.targetIdx != -1 && data.targetIdx < (int)data.Player_Turn.size()) {
                    target = &data.Player_Turn[data.targetIdx];
                }

                // 攻撃計算
                TotalAttack attackData = CalculateTotalAttack(data, attacker);

                Card* defenseCard = nullptr;
                // target が存在する場合のみ防御カードを処理
                if (target && !data.selectedDefenseCards.empty() && data.selectedDefenseCards[0] < target->Hand.GetCount()) {
                    defenseCard = const_cast<Card*>(&target->Hand.GetCards()[data.selectedDefenseCards[0]]);
                }

                // ResolveDamage も target がいる場合のみ、あるいはポインタ渡しに変更する
                if (target) {
                    ResolveDamage(data, *target, attackData, defenseCard);
                }

                // 計算が終わったらダメージ演出フェーズへ
                data.currentPhase = BattlePhase::Damage;
                data.animationTimer = 90;
            }
            else if (data.currentPhase == BattlePhase::Damage) {
                // --- 使ったカードの破棄とドロー処理 ---
                Player& attacker = data.Player_Turn[data.currentTurnIdx];
                Player& target = data.Player_Turn[data.targetIdx];

                // 攻撃側のカード破棄
                std::sort(data.selectedCards.rbegin(), data.selectedCards.rend());
                for (int idx : data.selectedCards) {
                    attacker.Hand.Remove(idx);
                }

                // 防御側のカード破棄
                std::sort(data.selectedDefenseCards.rbegin(), data.selectedDefenseCards.rend());
                for (int idx : data.selectedDefenseCards) {
                    target.Hand.Remove(idx);
                }

                // CardDB.GetRandomCard() を呼び出すようにする
                // ドロー処理（上限まで引く）
                while (attacker.Hand.GetCount() < CARD_MAX) {
                    attacker.Hand.Add(CardDB.GetRandomCard());
                }

                if (data.targetIdx != -1 && !target.Status.dead) {
                    while (target.Hand.GetCount() < CARD_MAX) {
                        target.Hand.Add(CardDB.GetRandomCard());
                    }
                }

                // 死亡判定
                if (data.Player_Turn[data.targetIdx].Status.dead) {
                    RemovePlayer(data, data.targetIdx);
                }

                // 次のターンへの準備
                NextTurn(data);
            }
        }
    }
}

void BattleLogicManager::NextTurn(BattleData& data) {
    if (data.Player_Turn.empty()) return;

    // 次のプレイヤーへ
    data.currentTurnIdx = (data.currentTurnIdx + 1) % data.Player_Turn.size();

    // 前のターンの選択情報を完全にリセット
    data.selectedCards.clear();
    data.selectedDefenseCards.clear();
    data.totalPower = 0;
    data.playerTarget = false;
    data.targetIdx = -1;
    data.revealIndex = 0;

    data.currentPhase = BattlePhase::Select;
}

void BattleLogicManager::RemovePlayer(BattleData& data, int targetIdx) {
    if (targetIdx < 0 || targetIdx >= (int)data.Player_Turn.size()) return;

    data.Player_Turn.erase(data.Player_Turn.begin() + targetIdx);

    // 削除によってターンの順番が狂わないように調整する
    if (targetIdx < data.currentTurnIdx) {
        data.currentTurnIdx--;
    }
    else if (data.currentTurnIdx >= (int)data.Player_Turn.size()) {
        data.currentTurnIdx = 0;
    }
}

TotalAttack BattleLogicManager::CalculateTotalAttack(BattleData& data, Player& attacker) {
    TotalAttack total;
    total.power = 0;
    total.isAll = false;
    total.type = _T("無");

    auto& hand = attacker.Hand.GetCards();

    if (data.selectedCards.empty()) return total;

    for (size_t i = 0; i < data.selectedCards.size(); ++i) {
        int index = data.selectedCards[i];
        if (index < 0 || index >= (int)hand.size()) continue;

        const Card& card = hand[index];

        if (i > 0 && card.GetCategory() != Attack) {
            continue; // 2枚目以降はAttackのみ許可
        }

        total.power += card.GetPower();

        if (card.GetCategory() == All) {
            total.isAll = true;
            total.hitPercent = card.GetPercent();
        }

        // ★属性の再計算ロジック
        std::string cardType = card.GetType();
        if (cardType == "") cardType = _T("無");

        if (cardType == _T("光")) {
            // 現在が無属性のときだけ光にする（1枚目が光の場合など）
            // すでに炎などになっていれば上書きしない（引き継ぐ）
            if (total.type == _T("無")) {
                total.type = _T("光");
            }
        }
        else if (cardType != _T("無")) {
            // 光・無以外の属性（炎、水、木、闇など）なら、その属性で上書きする
            total.type = cardType;
        }
    }

    return total;
}

void BattleLogicManager::ResolveDamage(BattleData& data, Player& target, const TotalAttack& attack, const Card* defenseCard) {

    // ダメージリゾルバーに「計算」だけを依頼する
    DamageResult result = DamageResolver::CalculateDamage(attack, defenseCard);

    // 結果に応じてプレイヤーのステータスを更新する（ロジックマネージャーの仕事）

    // もし回避(Miss)していたら、ダメージ処理はせず終わる
    if (!result.isHit) {
        // TODO: UIマネージャー側で「Miss!」と表示させるフラグをdataに立てるなど
        return;
    }

    // 闇属性による即死判定
    if (result.isInstantDeath) {
        target.setHp(0);
        target.Status.dead = true; // Status構造体にアクセス
        return;
    }

    // 通常のダメージ処理
    if (result.finalDamage > 0) {
        int currentHp = target.getHp();
        target.setHp(currentHp - result.finalDamage);

        // 死亡判定
        if (target.getHp() <= 0) {
            target.setHp(0);
            target.Status.dead = true; // Status構造体にアクセス
        }
    }

    // 完全にガードしきった（finalDamage == 0）時の処理もここに書けます
}

void BattleLogicManager::RecalculateAttackElement(BattleData& data, const std::vector<Card>& hand) {
    if (data.selectedCards.empty()) {
        data.currentAttackElement = _T("無");
        return;
    }

    std::string calculatedElement = _T("無"); // 初期値を「無」に設定

    for (size_t i = 0; i < data.selectedCards.size(); ++i) {
        std::string cardType = hand[data.selectedCards[i]].GetType();
        if (cardType == "") cardType = _T("無");

        // 属性の再計算ロジック
        if (cardType == _T("光")) {
            // 現在が無属性のときだけ光にする
            if (calculatedElement == _T("無")) {
                calculatedElement = _T("光");
            }
        }
        else if (cardType != _T("無")) {
            // 光・無以外の属性なら上書きする
            calculatedElement = cardType;
        }
    }

    data.currentAttackElement = calculatedElement;
}