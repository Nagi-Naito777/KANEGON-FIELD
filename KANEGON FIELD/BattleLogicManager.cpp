#include "BattleLogicManager.h"
#include "CardDatabase.h"
#include "Player.h"
#include <algorithm> // ソート用

void BattleLogicManager::Update(BattleData& data) {

    // 全体の汎用アニメーションフレームを更新
    data.animFrame++;

    // カード表示のアニメーション処理
    UpdateCardAnimation(data);

    // =============================================================
    // 各バトルのフェーズ進行(ステートマシン)
    // =============================================================
    switch (data.currentPhase) {
    case BattlePhase::Select:
    case BattlePhase::DefenseSelect:
        // 入力待ちフェーズ
        break;

    case BattlePhase::AttackReveal:
        if (data.animationTimer > 0) {
            data.animationTimer--;
        }
        else {
            if (data.revealIndex < data.selectedCards.size()) {
                data.revealIndex++;
                // UI用カウントを同期
                data.animAttackCardCount = data.revealIndex;
                data.animationTimer = 30; // 次のカードを開くまでの時間
            }
            else {
                // すべて公開し終わったらエフェクトフェーズへ
                data.currentPhase = BattlePhase::Effect;
                data.animationTimer = 60;   // ターゲット表示時間
            }
        }
        break;

    case BattlePhase::TargetDisplay:
        // ターゲット表示のウェイト
        if (data.animationTimer > 0) {
            data.animationTimer--;
        }
        else {
            // 防御カードの選択フェーズへ移行
            data.currentPhase = BattlePhase::DefenseSelect;
        }
        break;

    case BattlePhase::DefenseReveal:
        // 防御カードの公開演出
        if (data.animationTimer > 0) {
            data.animationTimer--;
        }
        else {
            if (data.revealIndex < (int)data.selectedDefenseCards.size()) {
                data.revealIndex++;
                // UI用カウントを同期
                data.animDefenseCardCount = data.revealIndex;
                data.animationTimer = 30; // 次の防御カードを開くまでの時間
            }
            else {
                // 公開完了後、ダメージ計算へ
                data.currentPhase = BattlePhase::Effect;
                data.animationTimer = 0;
            }
        }
        break;

    case BattlePhase::Effect: {
        // ダメージ計算の実行
        Player& attacker = data.Player_Turn[data.currentTurnIdx];
        Player* target = nullptr; // ポインタで保持する

        if (data.targetIdx != -1 && data.targetIdx < (int)data.Player_Turn.size()) {
            target = &data.Player_Turn[data.targetIdx];
        }

        // 攻撃計算
        TotalAttack attackData = CalculateTotalAttack(data, attacker);

        // 防御計算
        TotalDefense defenseData;
        if (target != nullptr && !data.selectedDefenseCards.empty()) {
            defenseData = CalculateTotalDefense(data, *target);
        }

        // ダメージ適用（ターゲットがいる場合のみ）
        if (target != nullptr) {
            ResolveDamage(data, *target, attackData, defenseData);
        }

        // 計算が終わったらダメージ演出フェーズへ
        data.currentPhase = BattlePhase::DamageResult;
        data.animationTimer = 90; // ダメージ表示時間（1.5秒）
        data.animFrame = 0;       // 演出用フレームをリセット
        break;
    }

    case BattlePhase::DamageResult:
        // ダメージ演出のコマ送り
        data.animFrame++;

        // ダメージ数の表示ウェイト
        if (data.animationTimer > 0) {
            data.animationTimer--;
        }
        else {
            // 時間が来たら終了処理へ
            data.currentPhase = BattlePhase::Idle;
        }
        break;

    case BattlePhase::Idle: {
        // 使ったカードの破棄とドロー・次ターンの準備
        Player& attacker = data.Player_Turn[data.currentTurnIdx];

        // 使用された奇跡カードの枚数をカウントする
        int miracleUsageCount = 0;
        for (int idx : data.selectedCards) {
            if (idx >= 0 && idx < (int)attacker.Hand.GetCards().size()) {
                if (attacker.Hand.GetCards()[idx].GetCategory() == CardCategory::Magic) {
                    miracleUsageCount++;
                }
            }
        }

        // 攻撃側のカード破棄
        std::sort(data.selectedCards.rbegin(), data.selectedCards.rend());
        int removedCount = 0;
        for (int idx : data.selectedCards) {
            const Card& card = attacker.Hand.GetCards()[idx];
            // 奇跡(Magic)以外なら破棄
            if (card.GetCategory() != CardCategory::Magic) {
                attacker.Hand.Remove(idx);
                removedCount++;
            }
        }

        // ドロー処理（破棄した分 ＋ 奇跡使用分）
        int totalDraw = removedCount + miracleUsageCount;
        for (int i = 0; i < totalDraw; ++i) {
            attacker.Hand.Add(CardDB.GetRandomCard());
        }
        attacker.Hand.Sort();

        // ターゲットが存在する場合のみ防御側の処理を行う
        if (data.targetIdx != -1 && data.targetIdx < (int)data.Player_Turn.size()) {
            Player& target = data.Player_Turn[data.targetIdx];

            // 防御側も同様に奇跡使用枚数をカウント
            int defMiracleCount = 0;
            for (int idx : data.selectedDefenseCards) {
                if (idx >= 0 && idx < (int)target.Hand.GetCards().size()) {
                    if (target.Hand.GetCards()[idx].GetCategory() == CardCategory::Magic) {
                        defMiracleCount++;
                    }
                }
            }

            // 防御側のカード破棄
            std::sort(data.selectedDefenseCards.rbegin(), data.selectedDefenseCards.rend());
            int defRemovedCount = 0;
            for (int idx : data.selectedDefenseCards) {
                const Card& card = target.Hand.GetCards()[idx];
                if (card.GetCategory() != CardCategory::Magic) {
                    target.Hand.Remove(idx);
                    defRemovedCount++;
                }
            }

            // 防御側のドロー処理
            if (!target.Status.dead) {
                int defTotalDraw = defRemovedCount + defMiracleCount;
                for (int i = 0; i < defTotalDraw; ++i) {
                    target.Hand.Add(CardDB.GetRandomCard());
                }
                target.Hand.Sort();
            }

            // 死亡判定
            if (target.Status.dead) {
                RemovePlayer(data, data.targetIdx);
            }
        }

        // NextTurn内でステートリセットと currentPhase = Select への移行が行われます
        NextTurn(data);
        break;
    }
    }
}

// 共通の属性加算関数
std::string BattleLogicManager::GetCombinedElement(const std::vector<int>& selectedIdxs, const std::vector<Card>& hand) {
    if (selectedIdxs.empty()) return "無";

    // 1枚目をベースにする
    std::string result = hand[selectedIdxs[0]].GetType();
    if (result == "") result = "無";

    // 2枚目以降の合成処理
    for (size_t i = 1; i < selectedIdxs.size(); ++i) {
        std::string addType = hand[selectedIdxs[i]].GetType();
        if (addType == "") addType = "無";

        if (result == addType) {
            // 同じ属性同士の加算(例 : 炎＋炎＝炎)はそのまま
            continue;
        }
        else if (addType == "光" && result != "闇") {
            // 追加が光属性で、ベースが闇以外なら元の属性を維持(例 : 炎＋光＝炎)
            continue;
        }
        else {
            // それ以外の異なる属性が混ざった場合は無属性になる
            result = "無";
        }
    }
    return result;
}

void BattleLogicManager::UpdateCardAnimation(BattleData& data) {
    float targetYOffset = 65.0f;
    size_t count = 0;

    // 現在のフェーズによってカウント対象を分岐
    if (data.currentPhase == BattlePhase::Select || data.currentPhase == BattlePhase::AttackReveal) {
        count = data.selectedCards.size();
    }
    else if (data.currentPhase == BattlePhase::DefenseSelect || data.currentPhase == BattlePhase::DefenseReveal) {
        count = data.selectedDefenseCards.size();
    }

    // アニメーション計算
    if (count > 0) {
        if (count >= 4) {
            targetYOffset = 30.0f;
        }
        data.currentYOffset += (targetYOffset - data.currentYOffset) * 0.1f;
    }
    else {
        data.currentYOffset = 65.0f;
    }
}

void BattleLogicManager::NextTurn(BattleData& data) {
    if (data.Player_Turn.empty()) return;

    // 次のプレイヤーへ
    data.currentTurnIdx = (data.currentTurnIdx + 1) % data.Player_Turn.size();

    // 前のターンの選択情報を完全にリセット
    data.selectedCards.clear();
    data.selectedDefenseCards.clear();
    data.attackTotalPower = 0;
    data.defenseTotalPower = 0;
    data.playerTarget = false;
    data.targetIdx = -1;
    data.revealIndex = 0;
    data.animAttackCardCount = 0;
    data.animDefenseCardCount = 0;

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

    auto& hand = attacker.Hand.GetCards();

    if (data.selectedCards.empty()) {
        total.type = _T("無");
        return total;
    }

    if (data.selectedCards.empty()) return total;
    if (data.selectedCards[0] < 0 || data.selectedCards[0] >= (int)hand.size()) return total;

    // 新しい共通関数で属性を一発取得
    total.type = GetCombinedElement(data.selectedCards, hand);

    for (size_t i = 0; i < data.selectedCards.size(); i++) {
        int index = data.selectedCards[i];
        if (index < 0 || index >= (int)hand.size()) continue;

        const Card& card = hand[index];

        // 2枚目以降はAttackのみ許可
        if (i > 0 && card.GetCategory() != Attack) {
            continue;
        }

        // 合計威力を計算
        total.power += card.GetPower();

        if (card.GetCategory() == All) {
            total.isAll = true;
            total.hitPercent = card.GetPercent();
        }
    }

    return total;
}

void BattleLogicManager::ResolveDamage(BattleData& data, Player& target, const TotalAttack& attack, const TotalDefense& defense) {
    // --- UIと同期するために属性をセット ---
    data.currentAttackElement = attack.type;

    // --- 命中判定 ---
    if (attack.isAll) {
        if ((rand() % 100) > attack.hitPercent) return;
    }

    int finalDamage = attack.power;

    // --- ガード判定（ここで防御カードのみを対象にする） ---
    if (defense.isActive) {
        // ここで「カテゴリが防御であること」を確認するガード節を追加
        if (DamageResolver::IsValidGuard(attack.type, defense.type)) {

            // ガード成功なら攻撃力から減算
            finalDamage -= defense.power;
        }
    }

    // ダメージがマイナスにならないように
    if (finalDamage < 0) finalDamage = 0;

    // 闇属性の特殊ルール
    if (attack.type == "闇") {
        if (finalDamage > 0) {
            target.setHp(0);
            target.Status.dead = true;
        }
    }
    else {
        // 通常ダメージ適用
        target.setHp(target.getHp() - finalDamage);
        if (target.getHp() <= 0) {
            target.setHp(0);
            target.Status.dead = true;
        }
    }
}

void BattleLogicManager::RecalculateAttackElement(BattleData& data, const std::vector<Card>& hand) {
    // 共通関数を使って簡略化
    data.currentAttackElement = GetCombinedElement(data.selectedCards, hand);
}

TotalDefense BattleLogicManager::CalculateTotalDefense(BattleData& data, Player& defender) {
    TotalDefense total;
    total.power = 0;
    total.isActive = false; // 初期化漏れ対策

    auto& hand = defender.Hand.GetCards();

    if (data.selectedDefenseCards.empty()) {
        total.type = _T("無");
        return total;
    }

    // 選んだカードが手札の範囲外なら強制リターン
    if (data.selectedDefenseCards[0] < 0 || data.selectedDefenseCards[0] >= (int)hand.size()) {
        total.type = _T("無");
        return total;
    }

    total.isActive = true;

    // 共通関数で属性を取得
    total.type = GetCombinedElement(data.selectedDefenseCards, hand);

    for (size_t i = 0; i < data.selectedDefenseCards.size(); ++i) {
        int index = data.selectedDefenseCards[i];
        if (index < 0 || index >= (int)hand.size()) continue;

        // 防御力を加算
        total.power += hand[index].GetPower();
    }
    return total;
}

void BattleLogicManager::RecalculateDefenseElement(BattleData& data, const std::vector<Card>& hand) {
    // 共通関数を使って簡略化
    data.currentDefenseElement = GetCombinedElement(data.selectedDefenseCards, hand);
}

// 防御カードの選択可否を判定（UI入力制限用）
bool BattleLogicManager::CanSelectDefenseCard(const BattleData& data, const Player& defender, int cardIdx, const std::string& incomingAttackElement) {
    auto& hand = defender.Hand.GetCards();
    if (cardIdx < 0 || cardIdx >= (int)hand.size()) return false;

    // 現在選択されている防御カードに、試しにクリックしたカードを追加してみる
    std::vector<int> tempSelected = data.selectedDefenseCards;
    tempSelected.push_back(cardIdx);

    // その状態での属性を計算
    std::string hypotheticalElement = GetCombinedElement(tempSelected, hand);

    // 出来上がった属性で、敵の攻撃を防げるかを判定
    return DamageResolver::IsValidGuard(incomingAttackElement, hypotheticalElement);
}