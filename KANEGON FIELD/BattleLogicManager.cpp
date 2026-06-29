#include "BattleLogicManager.h"
#include "CardDatabase.h"
#include "Player.h"
#include <algorithm> // ソート用

// カードの選択可否を更新するベース関数
void BattleLogicManager::UpdateCardSelectability(BattleData& data, const Player& player, bool isAttackTurn) {
    const auto& hand = player.Hand.GetCards();

    // isCardSelectableの配列サイズを手札の枚数に合わせる
    if (data.isCardSelectable.size() != hand.size()) {
        data.isCardSelectable.resize(hand.size(), true);
    }

    for (size_t i = 0; i < hand.size(); ++i) {
        const auto& card = hand[i];
        data.isCardSelectable[i] = true; // 基本は選択可能（明るい状態）として初期化

        // MP不足チェック（絶対選択不可）
        if (player.getMp() < card.GetMP()) {
            data.isCardSelectable[i] = false;
            continue;
        }

        // フェーズごとのカテゴリ・特定カードチェック
        if (data.currentPhase == BattlePhase::Select) {
            // 【攻撃フェーズ】
            // 防具カテゴリは不可
            if (card.GetCategory() == Defense) {
                data.isCardSelectable[i] = false;
            }
            // 防御系の奇跡（山、キャッスル等）も攻撃フェーズでは不可
            if (card.GetCategory() == Magic && (card.GetName() == "山" || card.GetName() == "キャッスル")) {
                data.isCardSelectable[i] = false;
            }
        }
        else if (data.currentPhase == BattlePhase::DefenseSelect) {
            // 【防御フェーズ】
            // 防御系の奇跡かどうかを判定
            bool isDefenseMagic = (card.GetCategory() == Magic && (card.GetName() == "山" || card.GetName() == "キャッスル"));

            // 防具、バイリンガル、防御系の奇跡「以外」は不可
            if (card.GetCategory() != Defense && card.GetCategory() != Bilingual && !isDefenseMagic) {
                data.isCardSelectable[i] = false;
            }
        }
    }
}

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
        ProcessSelectPhase(data);
        break;
    case BattlePhase::DefenseSelect:
        ProcessDefenseSelectPhase(data);
        break;
    case BattlePhase::AttackReveal:
        ProcessAttackRevealPhase(data);
        break;
    case BattlePhase::TargetDisplay:
        ProcessTargetDisplayPhase(data);
        break;
    case BattlePhase::DefenseReveal:
        ProcessDefenseRevealPhase(data);
        break;
    case BattlePhase::Effect:
        ProcessEffectPhase(data);
        break;
    case BattlePhase::DamageResult:
        ProcessDamageResultPhase(data);
        break;
    case BattlePhase::Idle:
        ProcessIdlePhase(data);
        break;
    case BattlePhase::ChangeStatusEdit:
        ProcessChangeStatusEditPhase(data);
        break;
    case BattlePhase::BuyConfirm:
        ProcessBuyConfirmPhase(data);
        break;
    case BattlePhase::End:
        break;
    }
}

// -------------------------------------------------------------
// 攻撃カード選択フェーズ
// -------------------------------------------------------------
void BattleLogicManager::ProcessSelectPhase(BattleData& data) {
    if (data.Player_Turn.empty()) return;

    Player& attacker = data.Player_Turn[data.currentTurnIdx];

    // 選択状態の自動切り替え
    if (data.selectedCards.size() >= 2) {
        int lastIdx = data.selectedCards.back(); // 最後にクリックされたカード
        const std::string& lastCardName = attacker.Hand.GetCards()[lastIdx].GetName();

        bool lastIsSell = (lastCardName == "バイバイ");
        bool lastIsSingle = (lastCardName == "チョイスチョイス" || lastCardName == "イコールイコール");

        int firstIdx = data.selectedCards.front(); // 最初に選ばれていたカード
        const std::string& firstCardName = attacker.Hand.GetCards()[firstIdx].GetName();

        bool firstIsSell = (firstCardName == "バイバイ");
        bool firstIsSingle = (firstCardName == "チョイスチョイス" || firstCardName == "イコールイコール");

        if (lastIsSell || lastIsSingle) {
            // あとから特殊カード(売・買・換)がクリックされた場合、これまでの選択を捨ててその特殊カードに切り替える
            data.selectedCards.clear();
            data.selectedCards.push_back(lastIdx);
        }
        else if (firstIsSingle) {
            // 先に「買」「換」を選択中に別のカードがクリックされた場合、新しいそのカードに切り替える
            data.selectedCards.clear();
            data.selectedCards.push_back(lastIdx);
        }
        else if (firstIsSell) {
            // 先に「売」を選択していて、新しいカード（売り物）がクリックされた場合
            // すでに「売＋売り物」が揃っている状態でさらに3枚目がクリックされたら、売り物を最新のものに切り替える
            if (data.selectedCards.size() >= 3) {
                data.selectedCards.clear();
                data.selectedCards.push_back(firstIdx); // 「売」は維持
                data.selectedCards.push_back(lastIdx);  // 最後に選んだものを新しい「売り物」にする
            }
        }
    }

    // カテゴリ等の基本制限を適用
    // ここで一旦、攻撃フェーズの基本ルール（防具不可など）が適用される
    UpdateCardSelectability(data, attacker, true);

    // 「売」カードが既に選択されているかチェック
    bool hasSellCard = false;
    for (int idx : data.selectedCards) {
        if (attacker.Hand.GetCards()[idx].GetName() == "バイバイ") {
            hasSellCard = true;
            break;
        }
    }

    // 現在の選択状態に応じた、他カードの選択可否更新
    for (size_t i = 0; i < attacker.Hand.GetCards().size(); ++i) {
        const Card& card = attacker.Hand.GetCards()[i];

        // すでに選択されているカード自身は、UIで再度クリックして解除できるよう選択可能にしておく
        auto it = std::find(data.selectedCards.begin(), data.selectedCards.end(), i);
        if (it != data.selectedCards.end()) {
            data.isCardSelectable[i] = true;
            continue;
        }

        if (hasSellCard) {
            // 【「売」モードの場合】
            if (data.selectedCards.size() >= 2) {
                // 既に「売＋売り物」の2枚が揃っているなら、他のカードはロックして暗くする
                data.isCardSelectable[i] = false;
            }
            else {
                // 売り物を選んでいない状態なら、すべてのカードを売り物候補として選択可能にする！
                // （UpdateCardSelectability で弾かれた防具やMP不足カードもここで true に上書き復活する）
                data.isCardSelectable[i] = true;
            }
        }
        else {
            // 【通常攻撃の加算モード または 「買」「換」モードの場合】

            // MP不足の奇跡カード制限
            if (!CanUseMiracleCard(attacker, card)) {
                data.isCardSelectable[i] = false;
                continue;
            }

            // 「買」「換」が選ばれている最中でも、他のカードをクリックして「切り替え」操作ができるようにするため、
            // isCardSelectable を false にしてロックすることはしません。
            // また、通常攻撃カードの加算も同様にロックしません。
        }
    }
}

// -------------------------------------------------------------
// 防御カード選択フェーズ
// -------------------------------------------------------------
void BattleLogicManager::ProcessDefenseSelectPhase(BattleData& data) {
    if (data.targetIdx < 0 || data.targetIdx >= (int)data.Player_Turn.size()) return;

    Player& defender = data.Player_Turn[data.targetIdx];

    // --- 属性相性やカテゴリ等の基本制限を適用 ---
    UpdateCardSelectability(data, defender, false);

    for (size_t i = 0; i < defender.Hand.GetCards().size(); ++i) {
        // 属性や回復処理中の制限
        if (!CanSelectDefenseCard(data, defender, i, data.currentAttackElement)) {
            data.isCardSelectable[i] = false;
            continue;
        }

        // MP不足の奇跡カード制限
        const Card& card = defender.Hand.GetCards()[i];
        if (!CanUseMiracleCard(defender, card)) {
            data.isCardSelectable[i] = false;
            continue;
        }

        // 選択済みカード制限
        auto it = std::find(data.selectedDefenseCards.begin(), data.selectedDefenseCards.end(), i);
        if (it != data.selectedDefenseCards.end()) {
            data.isCardSelectable[i] = false;
        }
    }
    // ここで入力待ちとなる
}

// -------------------------------------------------------------
// 攻撃カード公開演出フェーズ
// -------------------------------------------------------------
void BattleLogicManager::ProcessAttackRevealPhase(BattleData& data) {
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
            Player& attacker = data.Player_Turn[data.currentTurnIdx];

            // 最初に選んだカードを取得
            std::string firstCardName = "";
            if (!data.selectedCards.empty()) {
                firstCardName = attacker.Hand.GetCards()[data.selectedCards[0]].GetName();
            }

            // 換カードの場合、ステータス編集フェーズへ
            if (firstCardName == "イコールイコール") {
                data.currentPhase = BattlePhase::ChangeStatusEdit;
                return;
            }

            // 買カードの場合、ターゲットのカードを1枚抽出して確認フェーズへ
            if (firstCardName == "チョイスチョイス") {
                // ターゲット未指定ならランダムな他プレイヤーを選択
                if (data.targetIdx == -1 || data.targetIdx == data.currentTurnIdx) {
                    // ※実際は生存している自分以外のプレイヤーをランダムで選ぶ処理を入れてください
                    data.targetIdx = (data.currentTurnIdx + 1) % data.Player_Turn.size();
                }
                Player& target = data.Player_Turn[data.targetIdx];

                // 相手の手札からランダムに1枚選ぶ
                if (!target.Hand.GetCards().empty()) {
                    data.buyTargetCardIdx = rand() % target.Hand.GetCards().size();
                    int price = target.Hand.GetCards()[data.buyTargetCardIdx].GetMoney();

                    // お金が足りていれば購入確認、足りなければ演出（Effect）へ飛ばして不発にする
                    if (attacker.getMoney() >= price) {
                        data.currentPhase = BattlePhase::BuyConfirm;
                        return;
                    }
                }
                // 貧乏（買えない） or 相手の手札がない場合はそのままEffectフェーズへ
                data.currentPhase = BattlePhase::Effect;
                return;
            }

            // 回復カードが含まれているかチェック
            if (IsHealingAction(data, attacker)) {
                // ターゲットが自分の時はクリックせずにそのまま演出フェーズに
                if (IsSelfTarget(data)) {
                    data.currentPhase = BattlePhase::Effect;
                    data.animationTimer = 0;
                }
                // ターゲットを自分以外にした場合はその人が防御側のボタンを押すようにする
                else {
                    data.currentPhase = BattlePhase::DefenseSelect;
                    data.animationTimer = 0;
                }
            }
            else {
                data.currentPhase = BattlePhase::TargetDisplay;
                data.animationTimer = 60;
            }
        }
    }
}

// -------------------------------------------------------------
// ターゲット表示フェーズ
// -------------------------------------------------------------
void BattleLogicManager::ProcessTargetDisplayPhase(BattleData& data) {
    // ターゲット表示のウェイト
    if (data.animationTimer > 0) {
        data.animationTimer--;
    }
    else {
        // 防御カードの選択フェーズへ移行
        data.currentPhase = BattlePhase::DefenseSelect;
    }
}

// -------------------------------------------------------------
// 防御カード公開演出フェーズ
// -------------------------------------------------------------
void BattleLogicManager::ProcessDefenseRevealPhase(BattleData& data) {
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
}

// -------------------------------------------------------------
// ダメージ計算・効果処理フェーズ
// -------------------------------------------------------------
void BattleLogicManager::ProcessEffectPhase(BattleData& data) {
    // ダメージ計算の実行
    Player& attacker = data.Player_Turn[data.currentTurnIdx];

    // --- 攻撃対象（ダメージを受ける人）の設定 ---
    Player* target = nullptr; // ポインタで保持する
    if (data.targetIdx != -1 && data.targetIdx < (int)data.Player_Turn.size()) {
        target = &data.Player_Turn[data.targetIdx];
    }

    // 各種カード効果（奇跡や売買換など）の発動
    if (!data.isPendingAttack) {
        for (int idx : data.selectedCards) {
            const Card& card = attacker.Hand.GetCards()[idx];
            ExecuteCardEffect(data, attacker, target, card);
        }
    }

    if (target != nullptr) {
        for (int idx : data.selectedDefenseCards) {
            const Card& defCard = target->Hand.GetCards()[idx];
            ExecuteCardEffect(data, *target, &attacker, defCard);
        }
    }

    // 攻撃データの算出
    TotalAttack attackData;
    if (data.isPendingAttack) {
        // 【カウンター連鎖時】保留されていた攻撃データを使用する
        attackData.power = data.pendingAttackPower;
        attackData.type = data.pendingAttackType;
        attackData.isAll = false;
        data.isPendingAttack = false; // 計算に適用したので一旦下ろす
    }
    else {
        // 【通常時】手札から攻撃力を計算
        attackData = CalculateTotalAttack(data, attacker);
        // アンリミテッドの固定追加ダメージを加算
        attackData.power += data.extraAttackPower;
    }

    // 「ズ」の２倍効果をここで反映
    attackData.power = static_cast<int>(attackData.power * data.attackMultiplier);
    data.attackTotalPower = attackData.power;

    // 防御データの算出
    TotalDefense defenseData;
    if (target != nullptr && !data.selectedDefenseCards.empty()) {
        defenseData = CalculateTotalDefense(data, *target);
        data.defenseTotalPower = defenseData.power; // 防御側UI表示用
    }

    // ダメージ適用とカウンター判定
    if (target != nullptr) {
        ResolveDamage(data, attacker, *target, attackData, defenseData);
    }

    // カウンターが成立した場合は処理を抜ける
    if (data.currentPhase == BattlePhase::DefenseSelect) {
        return;
    }

    // 計算が終わったらダメージ演出フェーズへ
    data.currentPhase = BattlePhase::DamageResult;
    data.animationTimer = 90; // ダメージ表示時間（1.5秒）
    data.animFrame = 0;       // 演出用フレームをリセット
}

// -------------------------------------------------------------
// ダメージ結果表示フェーズ
// -------------------------------------------------------------
void BattleLogicManager::ProcessDamageResultPhase(BattleData& data) {
    // ダメージ数の表示ウェイト
    if (data.animationTimer > 0) {
        data.animationTimer--;
    }
    else {
        // 時間が来たら終了処理へ
        data.currentPhase = BattlePhase::Idle;
    }
}

// -------------------------------------------------------------
// ターン終了・クリーンアップフェーズ
// -------------------------------------------------------------
void BattleLogicManager::ProcessIdlePhase(BattleData& data) {
    // 使ったカードの破棄とドロー・次ターンの準備
    Player& attacker = data.Player_Turn[data.currentTurnIdx];

    // 使用された奇跡カードの枚数をカウントする
    int miracleUsageCount = 0;
    for (int idx : data.selectedCards) {
        if (idx >= 0 && idx < (int)attacker.Hand.GetCards().size()) {
            const Card& card = attacker.Hand.GetCards()[idx];
            if (attacker.Hand.GetCards()[idx].GetCategory() == CardCategory::Magic) {
                // アンリミテッド等で0になった後にマイナスにならないよう max でガード
                attacker.setMp((std::max)(0, attacker.getMp() - card.GetMP()));
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
                const Card& card = target.Hand.GetCards()[idx];
                if (card.GetCategory() == CardCategory::Magic) {
                    // 奇跡カードのMP消費
                    target.setMp(target.getMp() - card.GetMP());
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

    NextTurn(data);
}

void BattleLogicManager::ProcessChangeStatusEditPhase(BattleData& data) {
    // UI側で増減を行い、決定ボタン（ATTACKなど）が押されたら次へ
    if (data.selectedOption == BattleOption::ATTACK) {
        data.selectedOption = BattleOption::NONE;
        data.currentPhase = BattlePhase::Effect; // 効果適用へ
    }
}

void BattleLogicManager::ProcessBuyConfirmPhase(BattleData& data) {
    // UI側の「買う」「買わない」入力待ち
    if (data.selectedOption == BattleOption::BUY_YES || data.selectedOption == BattleOption::BUY_NO) {
        data.currentPhase = BattlePhase::Effect;
        // 実際の売買ロジックは ExecuteCardEffect 内で行う
    }
}

// --- MPが足りているかチェックする関数 (UI制限・表示用) ---
bool BattleLogicManager::CanUseMiracleCard(const Player& player, const Card& card) {
    if (card.GetCategory() != CardCategory::Magic) return true; // 奇跡以外は常にtrue
    return player.getMp() >= card.GetMP();
}

// 判定用のヘルパー関数
bool BattleLogicManager::IsHealingAction(const BattleData& data, const Player& attacker) {
    for (int idx : data.selectedCards) {
        if (idx < 0 || idx >= (int)attacker.Hand.GetCards().size()) continue;
        CardCategory cat = attacker.Hand.GetCards()[idx].GetCategory();
        if (cat == CardCategory::Healing || cat == CardCategory::MagicHealing) {
            return true;
        }
    }
    return false;
}

bool BattleLogicManager::IsSelfTarget(const BattleData& data) {
    // ターゲットが自分自身のID、または「未選択(-1)」の場合は自分と判定する
    return data.targetIdx == data.currentTurnIdx || data.targetIdx == -1;
}

void BattleLogicManager::ExecuteCardEffect(BattleData& data, Player& attacker, Player* target, const Card& card) {
    std::string name = card.GetName();

    // 換・買・売カードの処理
    if (name == "イコールイコール") {
        // MPと金額を増減させ、その合計値をHPから引く（マイナスなら回復する）
        attacker.setMp(attacker.getMp() + data.changeMP);
        attacker.setMoney(attacker.getMoney() + data.changeMoney);
        int hpCost = data.changeMP + data.changeMoney;
        attacker.setHp(attacker.getHp() - hpCost);
    }
    else if (name == "チョイスチョイス") {
        if (data.selectedOption == BattleOption::BUY_YES && target != nullptr && data.buyTargetCardIdx != -1) {
            const Card& buyCard = target->Hand.GetCards()[data.buyTargetCardIdx];
            int price = buyCard.GetMoney(); // 価格取得

            // お金のやり取り（PlayerクラスにsetMoney等のメソッドがある前提）
            attacker.setMoney(attacker.getMoney() - price);
            target->setMoney(target->getMoney() + price);

            // カードの移動
            attacker.Hand.Add(buyCard);
            target->Hand.Remove(data.buyTargetCardIdx);

            // 買ったので、次のカード破棄処理で不具合が出ないようインデックスをリセット
            data.buyTargetCardIdx = -1;
        }
    }
    else if (name == "バイバイ" && target != nullptr && data.selectedCards.size() >= 2) {
        // 選ばれた2枚目のカード（売りつけるアイテム）を取得
        int sellItemIdx = data.selectedCards[1];
        const Card& sellCard = attacker.Hand.GetCards()[sellItemIdx];

        int price = sellCard.GetMoney();
        if (sellCard.GetCategory() == CardCategory::Magic) price = 0; // 奇跡は0円

        // お金 -> MP -> HP の順にダメージ
        int remainingDamage = price;

        // お金から引く
        int moneyDeduct = (std::min)(target->getMoney(), remainingDamage);
        target->setMoney(target->getMoney() - moneyDeduct);
        remainingDamage -= moneyDeduct;

        // MPから引く
        if (remainingDamage > 0) {
            int mpDeduct = (std::min)(target->getMp(), remainingDamage);
            target->setMp(target->getMp() - mpDeduct);
            remainingDamage -= mpDeduct;
        }

        // HPから引く
        if (remainingDamage > 0) {
            target->setHp(target->getHp() - remainingDamage);
            if (target->getHp() <= 0) {
                target->setHp(0);
                target->Status.dead = true;
            }
        }
    }

    // カテゴリで大きく分岐
    if (card.GetCategory() == CardCategory::Healing) {
        if (target != nullptr) {
            target->setHp(target->getHp() + card.GetPower());

            // 回復量を記録(UI用データ保持)
            data.lastHealingDone += card.GetPower();
            data.resultTargetIdx = data.targetIdx;
        }
    }
    // MP回復カードの処理
    else if (card.GetCategory() == CardCategory::MagicHealing) {
        if (target != nullptr) {
            // MPを回復させる (※必要に応じて最大MPを超えないような上限チェックをここに入れます)
            target->setMp(target->getMp() + card.GetPower());

            // UI側に回復量を表示するための記録（HP回復と共用にするかはお好みで調整してください）
            data.lastHealingDone += card.GetPower();
            data.resultTargetIdx = data.targetIdx;
        }
    }
    else if (card.GetCategory() == CardCategory::Magic) {
        // 奇跡カードの個別処理（名前で分岐）
        std::string name = card.GetName();

        // 全体攻撃化奇跡(攻撃フェーズのみ)
        if (name == "クラス") {
            data.isAllAttack = true;
        }
        // 攻撃を2倍にする奇跡(攻撃)
        else if (name == "ズ") {
            // 重ね掛けも考慮して乗算にする（初期値は1.0fにしておく）
            data.attackMultiplier *= 2.0f;
        }
        // 無属性攻撃カードのみ無効化(防御フェーズのみ)
        else if (name == "キャッスル") {
            data.isImmune = true;
        }
        // 相手のHPを吸収(攻撃)
        else if (name == "スティール") {
            // 吸収フラグ
            data.isDrain = true;
        }
        // 奇跡カードのみ確率で弾く(防御)
        else if (name == "山") {
            // 50%の確率で弾くフラグを立てる
            if ((rand() % 100) < 50) {
                data.isParry = true;
            }
        }
        // 自身のMPを全て消費して２倍の攻撃力として攻撃する(攻撃)
        else if (name == "アンリミテッド") {
            // 現在のMP×2を「追加攻撃力」として保持してからMPを0にする
            data.extraAttackPower += (attacker.getMp() * 2);
            attacker.setMp(0);
        }
    }
    // 攻撃カードや防御カードにも特殊効果を持たせたい場合
    else if (card.GetCategory() == CardCategory::Attack) {
        std::string name = card.GetName();
    }
}

// UI表示用の文字列を返す関数
std::string BattleLogicManager::GetCardEffectDescription(const Card& card) {
    std::string name = card.GetName();
    CardCategory cat = card.GetCategory();

    // 回復系の特殊効果文章を返す処理
    if (cat == CardCategory::Healing || cat == CardCategory::MagicHealing) {

    }

    // 奇跡系はカードごとの説明文
    if (cat == CardCategory::Magic) {
        if (name == "クラス") return card.GetDescription();
        if (name == "ズ") return card.GetDescription();
        if (name == "キャッスル") return card.GetDescription();
        if (name == "スティール") return card.GetDescription();
        if (name == "山") return card.GetDescription();
        if (name == "アンリミテッド") return card.GetDescription();
    }

    // 攻撃系の追加説明
    if (cat == CardCategory::Attack) {

    }

    // 防御系の追加説明
    if (cat == CardCategory::Defense) {

    }

    return "効果なし";
}

// 共通の属性加算関数
std::string BattleLogicManager::GetCombinedElement(const std::vector<int>& selectedIdxs, const std::vector<Card>& hand) {
    if (selectedIdxs.empty()) return "無";

    // 1枚目で初期化
    std::string result = hand[selectedIdxs[0]].GetType();
    if (result == "") result = "無";

    for (size_t i = 1; i < selectedIdxs.size(); ++i) {
        std::string next = hand[selectedIdxs[i]].GetType();
        if (next == "") next = "無";

        // 同属性なら何も起きない（継続）
        if (result == next) continue;

        // 闇と光の混合は無条件で無属性
        if ((result == "光" && next == "闇") || (result == "闇" && next == "光")) {
            return "無";
        }

        // 光属性が含まれる場合の特殊処理
        // result が光なら、次の属性に上書きされる（光＋炎＝炎）
        if (result == "光") {
            result = next;
            continue;
        }
        // next が光なら、現在の属性が維持される（炎＋光＝炎）
        if (next == "光") {
            continue;
        }

        // 上記以外（異なる属性同士の混合）はすべて無
        return "無";
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

    // 前のターンの選択情報、および【UI用確定リザルト】を完全にリセット
    data.selectedCards.clear();
    data.selectedDefenseCards.clear();
    data.attackTotalPower = 0;
    data.defenseTotalPower = 0;
    data.playerTarget = false;
    data.targetIdx = -1;
    data.revealIndex = 0;
    data.animAttackCardCount = 0;
    data.animDefenseCardCount = 0;

    // 属性や特殊効果フラグ、倍率などを完全に初期化
    data.currentAttackElement = "無";
    data.currentDefenseElement = "無";
    data.attackMultiplier = 1.0f;
    data.isAllAttack = false;
    data.isImmune = false;
    data.isParry = false;
    data.isDrain = false;

    // UI用リザルトのリセット
    data.lastDamageDealt = 0;
    data.lastHealingDone = 0;
    data.resultTargetIdx = -1;

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
        total.type = "無";
        return total;
    }

    if (data.selectedCards[0] < 0 || data.selectedCards[0] >= (int)hand.size()) return total;

    // 特殊カード（売・買・換）の場合は計算をスキップ
    const std::string& firstCardName = hand[data.selectedCards[0]].GetName();
    if (firstCardName == "バイバイ" || firstCardName == "チョイスチョイス" || firstCardName == "イコールイコール") {
        total.type = "無";   // 特殊カードは常に無属性扱い
        total.power = 0;     // ダメージ計算には使わないので0
        total.isAll = false; // 全体攻撃カードを売る際のエラーを防ぐ
        return total;
    }

    // 新しい共通関数で属性を一発取得
    total.type = GetCombinedElement(data.selectedCards, hand);

    for (size_t i = 0; i < data.selectedCards.size(); i++) {
        int index = data.selectedCards[i];
        if (index < 0 || index >= (int)hand.size()) continue;

        const Card& card = hand[index];

        // 回復系カードは攻撃計算から除外する
        if (card.GetCategory() == Healing || card.GetCategory() == MagicHealing) {
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

void BattleLogicManager::ResolveDamage(BattleData& data, Player& attacker, Player& target,
    const TotalAttack& attack, const TotalDefense& defense) {
    // --- UIと同期するために属性をセット ---
    data.currentAttackElement = attack.type;

    // --- 命中判定 ---
    if (attack.isAll) {
        if ((rand() % 100) > attack.hitPercent) return;
    }

    // 倍率を適用した攻撃力 (暗黙の型変換を防ぐため明示的にintへキャスト)
    int incomingDamage = static_cast<int>(attack.power * data.attackMultiplier);

    // --- カウンター・弾き返し処理 ---
    // 弾かれた（保留された）攻撃であれば無条件で魔法扱いにする
    bool hasMagicAttack = data.isPendingAttack;
    if (!hasMagicAttack) {
        for (int idx : data.selectedCards) {
            if (attacker.Hand.GetCards()[idx].GetCategory() == CardCategory::Magic) {
                hasMagicAttack = true;
                break;
            }
        }
    }

    // 「山」の条件成立時
    if (data.isParry && hasMagicAttack) {
        data.popups.push_back({ PopupType::Text, data.targetIdx, "弾いた！", 0x00FF00, 0, 60 });

        // カウンター攻撃として、受けるはずだったダメージと属性を「保留」にする
        data.isPendingAttack = true;
        // 防具で減算される「前」の威力を跳ね返す
        data.pendingAttackPower = incomingDamage;
        data.pendingAttackType = attack.type;

        // 初回のカウンターなら、本来のターンプレイヤー(元アタッカー)を退避
        if (data.originalTurnIdx == -1) {
            data.originalTurnIdx = data.currentTurnIdx;
        }

        // 攻守の完全入れ替え
        int tempIdx = data.currentTurnIdx;
        data.currentTurnIdx = data.targetIdx; // 弾いた人が新たな「アタッカー」に
        data.targetIdx = tempIdx;            // 元アタッカーが新たな「ディフェンダー」に

        // カード選択状態を初期化して、次のキャッチボールに備える
        data.selectedCards.clear();
        data.selectedDefenseCards.clear();
        data.revealIndex = 0;
        data.animAttackCardCount = 0;
        data.animDefenseCardCount = 0;

        // 一時フラグのリセット（次のカウンター連鎖で再判定するため）
        data.isImmune = false;
        data.isParry = false;
        data.isDrain = false;
        data.attackMultiplier = 1.0f;
        data.isAllAttack = false;

        // 防御カード選択フェーズへ「逆戻り」させる
        data.currentPhase = BattlePhase::DefenseSelect;
        return;
    }

    // --- ガード判定 ---
    int finalDamage = incomingDamage;
    if (defense.isActive) {
        if (DamageResolver::IsValidGuard(attack.type, defense.type)) {
            finalDamage -= defense.power;
        }
    }

    // 無属性攻撃を無効化
    if (data.isImmune && attack.type == "無") {
        data.popups.push_back({ PopupType::Text, data.targetIdx, "無効！", 0xAAAAAA, 0, 60 });
        return;
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
        data.popups.emplace_back(PopupType::Number, data.targetIdx, std::to_string(finalDamage), 0xFF0000, 0, 60);

        // スティール（HP吸収処理）
        if (data.isDrain && finalDamage > 0) {
            attacker.setHp(attacker.getHp() + finalDamage);
            data.popups.emplace_back(PopupType::Number, data.targetIdx, std::to_string(finalDamage), 0xFF0000, 0, 60);
        }
    }
}

void BattleLogicManager::RecalculateAttackElement(BattleData& data, const std::vector<Card>& hand) {
    if (!data.selectedCards.empty()) {
        int firstIdx = data.selectedCards[0];
        if (firstIdx >= 0 && firstIdx < (int)hand.size()) {
            const std::string& firstName = hand[firstIdx].GetName();
            if (firstName == "バイバイ" || firstName == "チョイスチョイス" || firstName == "イコールイコール") {
                data.currentAttackElement = "無";
                return;
            }
        }
    }

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
    // 防御カードの選択可否フラグを全リセット
    std::fill(data.isCardSelectable.begin(), data.isCardSelectable.end(), false);

    // 現在の「相手の攻撃属性」を取得
    std::string attackElem = data.currentAttackElement;

    // 全カードに対して、この攻撃属性を防げるか判定
    for (int i = 0; i < (int)hand.size(); ++i) {
        std::string defenseElem = hand[i].GetType();

        // 【防御ロジックの核】
        // IsValidGuard に判定を統一
        if (DamageResolver::IsValidGuard(attackElem, defenseElem)) {
            data.isCardSelectable[i] = true;
        }
    }
}

// 防御カードの選択可否を判定（UI入力制限用）
bool BattleLogicManager::CanSelectDefenseCard(const BattleData& data, const Player& defender, int cardIdx, const std::string& incomingAttackElement) {
    // 回復アクション中の場合、防御カード選択をブロックする
    // 攻撃者(ターンプレイヤー)のカードを確認する必要がある
    const Player& attacker = data.Player_Turn[data.currentTurnIdx];
    if (IsHealingAction(data, attacker)) {
        return false;
    }

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