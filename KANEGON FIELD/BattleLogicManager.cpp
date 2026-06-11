#include "BattleLogicManager.h"
#include "CardDatabase.h"
#include "Player.h"
#include <algorithm> // ソート用

void BattleLogicManager::Update(BattleData& data) {

    // カード表示のアニメーション処理
    UpdateCardAnimation(data);

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

                // 防御計算
                TotalDefense defenseData;
                if (target != nullptr && !data.selectedDefenseCards.empty()) {
                    defenseData.isActive = true;

                    // 全手札のリストを取得
                    const std::vector<Card>& cards = target->Hand.GetCards();

                    // 選択されたインデックスを使って、特定のカードを参照する
                    // インデックスの範囲外アクセスを防ぐため、念のためサイズチェック
                    int index = data.selectedDefenseCards[0];
                    if (index >= 0 && index < (int)cards.size()) {
                        const Card& defCard = cards[index];
                        defenseData.power = defCard.GetPower();
                        defenseData.type = defCard.GetType();
                    }
                }

                // ダメージ適用（ターゲットがいる場合のみ）
                if (target != nullptr) {
                    ResolveDamage(data, *target, attackData, defenseData);
                }

                // 計算が終わったらダメージ演出フェーズへ
                data.currentPhase = BattlePhase::Damage;
                data.animationTimer = 90;
            }
            else if (data.currentPhase == BattlePhase::Damage) {
                // --- 使ったカードの破棄とドロー処理 ---
                Player& attacker = data.Player_Turn[data.currentTurnIdx];

                // 使用された奇跡カードの枚数をカウントする
                int miracleUsageCount = 0;
                for (int idx : data.selectedCards) {
                    // インデックスの範囲チェックをしてからカテゴリを確認
                    if (idx >= 0 && idx < (int)attacker.Hand.GetCards().size()) {
                        if (attacker.Hand.GetCards()[idx].GetCategory() == CardCategory::Magic) {
                            miracleUsageCount++;
                        }
                    }
                }

                // 攻撃側のカード破棄
                std::sort(data.selectedCards.rbegin(), data.selectedCards.rend());
                
                int removedCount = 0; // 破棄した枚数
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

                // 手札の並び替え（奇跡カードを後ろにするためのソート）
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

                    // 防御側のカード破棄(防御カードも同様に奇跡カード以外を削除)
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
                        target.Hand.Sort(); // 防御側も並び替え処理を置く
                    }

                    // 死亡判定
                    if (target.Status.dead) {
                        RemovePlayer(data, data.targetIdx);
                    }
                }

                // --- ステートのリセット（重要） ---
                data.selectedCards.clear();
                data.selectedDefenseCards.clear();
                data.playerTarget = false;
                data.targetIdx = -1;

                // 次のターンへの準備
                data.currentPhase = BattlePhase::Select;
                NextTurn(data);
            }
        }
    }
}

void BattleLogicManager::UpdateCardAnimation(BattleData& data) {
    float targetYOffset = 65.0f;
    size_t count = 0;

    // 現在のフェーズによってカウント対象を分岐
    if (data.currentPhase == BattlePhase::Select) {
        count = data.selectedCards.size();
    }
    else if (data.currentPhase == BattlePhase::DefenseSelect) {
        count = data.selectedDefenseCards.size();
    }
    // 【重要】Reveal中も「選択していたカード」を表示するはずなので、ここでもカウントが必要
    else if (data.currentPhase == BattlePhase::Reveal) {
        count = data.selectedCards.size();
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

    auto& hand = attacker.Hand.GetCards();

    if (data.selectedCards.empty()) {
        total.type = _T("無");
        return total;
    }

    if (data.selectedCards.empty()) return total;
    if (data.selectedCards[0] < 0 || data.selectedCards[0] >= (int)hand.size()) return total;

    // 1枚目の属性をベースにする
    std::string currentElement = hand[data.selectedCards[0]].GetType();
    if (currentElement == "") currentElement = _T("無");

    bool hasNonLightAddition = false;

    for (size_t i = 0; i < data.selectedCards.size(); i++) {
        int index = data.selectedCards[i];
        if (index < 0 || index >= (int)hand.size()) continue;

        const Card& card = hand[index];

        // 2枚目以降はAttackのみ許可
        if (i > 0 && card.GetCategory() != Attack) {
            continue;
        }

        // 威力を加算
        total.power += card.GetPower();

        // 全体攻撃(All)の判定
        if (card.GetCategory() == All) {
            total.isAll = true;
            total.hitPercent = card.GetPercent();
        }

        // 属性チェック
        if (i > 0) {
            std::string addType = card.GetType();
            if (addType == "") addType = _T("無");
            if (addType != _T("光")) {
                hasNonLightAddition = true;
            }
        }
    }

    // 最終的な属性を適用
    total.type = currentElement;

    return total;
}

void BattleLogicManager::ResolveDamage(BattleData& data, Player& target, const TotalAttack& attack, const TotalDefense& defense) {
    
    // 命中判定（全体攻撃などのフラグ判定）
    if (attack.isAll) {
        if ((rand() % 100) > attack.hitPercent) {
            // Miss! ダメージ処理を行わず終了
            return;
        }
    }

    int finalDamage = attack.power;

    // ガード判定ロジック
    // defense構造体のフラグと属性を使用して判定
    if (defense.isActive) {
        // DamageResolverの相性チェックを呼び出す
        if (DamageResolver::IsValidGuard(attack.type, defense.type)) {
            finalDamage = (std::max)(0, finalDamage - defense.power);
        }
        else {
            // ガード失敗（相性不良）：貫通。ダメージはそのまま
        }
    }

    // ダメージがマイナスにならないように
    if (finalDamage < 0) finalDamage = 0;

    // 3. 闇属性の特殊ルール
    if (attack.type == "闇") {
        if (finalDamage > 0) {
            target.setHp(0);
            target.Status.dead = true;
        }
    }
    else {
        // 4. 通常ダメージ適用
        target.setHp(target.getHp() - finalDamage);
        if (target.getHp() <= 0) {
            target.setHp(0);
            target.Status.dead = true;
        }
    }
}

void BattleLogicManager::RecalculateAttackElement(BattleData& data, const std::vector<Card>& hand) {
    if (data.selectedCards.empty()) {
        data.currentAttackElement = _T("無");
        return;
    }

    // 1枚目をベースにする（念のためインデックス範囲チェック）
    int firstIdx = data.selectedCards[0];
    if (firstIdx < 0 || firstIdx >= (int)hand.size()) {
        data.currentAttackElement = _T("無");
        return;
    }

    std::string currentElement = hand[firstIdx].GetType();
    if (currentElement == "") currentElement = _T("無");

    bool hasNonLightAddition = false;

    // 2枚目以降をチェックして状態を更新していく
    for (size_t i = 1; i < data.selectedCards.size(); ++i) {
        std::string addType = hand[data.selectedCards[i]].GetType();
        if (addType == "") addType = _T("無");

        if (addType != _T("光")) {
            hasNonLightAddition = true;
        }
    }

    // 最終的な表示属性を決定
    if (hasNonLightAddition) {
        data.currentAttackElement = _T("無");
    }
    else {
        data.currentAttackElement = currentElement;
    }
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

    // ★安全ガード：選んだカードが手札の範囲外なら強制リターン
    if (data.selectedDefenseCards[0] < 0 || data.selectedDefenseCards[0] >= (int)hand.size()) {
        total.type = _T("無");
        return total;
    }

    total.isActive = true;

    // 1枚目をベースにする
    std::string currentElement = hand[data.selectedDefenseCards[0]].GetType();
    if (currentElement == "") currentElement = _T("無");

    // 選択された防御カードを順に処理
    for (size_t i = 0; i < data.selectedDefenseCards.size(); ++i) {
        int index = data.selectedDefenseCards[i];
        if (index < 0 || index >= (int)hand.size()) continue;

        const Card& card = hand[index];

        // 防御力を加算
        total.power += card.GetPower();

        // 2枚目以降の属性チェック
        if (i > 0) {
            std::string addType = card.GetType();
            if (addType == "") addType = _T("無");

            if (addType == currentElement) {
                // 同じ属性同士はそのまま維持
            }
            else if (addType == _T("光")) {
                // 追加が「光」の場合、現在の属性をそのまま維持
            }
            else if (currentElement == _T("光")) {
                // 現在が「光」で、追加が「光以外」なら上書き
                currentElement = addType;
            }
            else {
                // 違う属性が混ざった場合は無になる
                currentElement = _T("無");
            }
        }
    }

    total.type = currentElement;
    return total;
}

void BattleLogicManager::RecalculateDefenseElement(BattleData& data, const std::vector<Card>& hand) {
    // 選択された防御カードがない場合は無属性
    if (data.selectedDefenseCards.empty()) {
        data.currentDefenseElement = _T("無");
        return;
    }

    // 安全ガード
    if (data.selectedCards[0] < 0 || data.selectedCards[0] >= (int)hand.size()) return;

    // 防御の場合は1枚目（ベース）の属性を取得
    std::string element = hand[data.selectedDefenseCards[0]].GetType();
    if (element == "") element = _T("無");

    // 2枚目以降をチェックして状態を更新していく
    for (size_t i = 1; i < data.selectedDefenseCards.size(); ++i) {
        std::string addType = hand[data.selectedDefenseCards[i]].GetType();
        if (addType == "") addType = _T("無");

        if (addType == element) {
            // 同じ属性同士はそのまま維持
        }
        else if (addType == _T("光")) {
            // 追加が「光」の場合、現在の属性（炎、水、無など）をそのまま維持
        }
        else if (element == _T("光")) {
            // 現在が「光」で、追加が「光以外」なら上書き（光＋炎＝炎、光＋無＝無）
            element = addType;
        }
        else {
            // 違う属性が混ざった場合は無になる
            element = _T("無");
        }
    }

    data.currentDefenseElement = element;
}