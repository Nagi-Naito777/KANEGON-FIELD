#include "BattleInputManager.h"
#include "BattleData.h"
#include "BattleLogicManager.h"
#include "Card.h"
#include "Player.h"
#include <algorithm>
#include <vector>

// 更新処理
PlayerAction BattleInputManager::Update(BattleData& data, const InputManager& input, Player& humanPlayer, int humanIdx, bool isHumanTurn) {
    PlayerAction action; // 今回のフレームでの行動結果

    // 毎フレームの初期化
    std::fill(std::begin(data.isHoverIdx), std::end(data.isHoverIdx), false);
    std::fill(std::begin(data.isHoverPlayerIdx), std::end(data.isHoverPlayerIdx), false);
    std::fill(std::begin(data.isHoverCardIdx), std::end(data.isHoverCardIdx), false);
    data.hoveredCardIdx = -1;

    bool wasSurrenderConfirm = data.isSurrenderConfirm;

    // 降参UIの処理
    ProcessSurrender(data, input, action);
    if (action.isSurrender) {
        return action; // 降参確定ならすぐ返す
    }

    if (wasSurrenderConfirm || data.isSurrenderConfirm) {
        return action; // ウィンドウが開いていたら他の入力は無視
    }

    // 攻撃/防御の決定ボタン処理
    ProcessActionButtons(data, input, humanIdx, isHumanTurn, action);

    // ターゲットの手動選択処理 (ローカル処理なのでそのまま)
    ProcessTargetSelection(data, input, isHumanTurn);

    // 手札の選択・コンボ処理 (ローカル処理なのでそのまま)
    ProcessHandSelection(data, input, humanPlayer, humanIdx, isHumanTurn);

    return action;
}

// -------------------------------------------------------------
// 降参UIの処理
// -------------------------------------------------------------
void BattleInputManager::ProcessSurrender(BattleData& data, const InputManager& input, PlayerAction& action) {
    if (data.isSurrenderConfirm) {
        // ギブアップのボタン描画用変数
        int give_butX = 425;
        int give_butY = 300;
        int give_w = 150;
        int give_h = 50;

        // マウス判定
        data.isHoverIdx[BattleOption::GIVE_UP] = input.IsMouseOver(give_butX, give_butY, give_w, give_h);

        // 確認画面を閉じる条件（再度戻るボタンを押す or ウィンドウ外をクリック）
        bool clickedReturnAgain = (input.IsLeftClicked() && input.IsMouseOver(RET_BUT_X, RET_BUT_Y, RET_BUTW, RET_BUTH));
        bool clickedOutside = (input.IsLeftClicked() && !input.IsMouseOver(300, 200, 400, 200));

        if (clickedReturnAgain || clickedOutside) {
            data.isSurrenderConfirm = false;
        }
        else if (input.IsLeftClicked() && data.isHoverIdx[BattleOption::GIVE_UP]) {
            // 降参決定時のデータリセット
            data.isSurrenderConfirm = false;
            action.isSurrender = true; // ★直接終了せず、アクションとして通知する
            action.hasAction = true;
        }
        return;
    }

    // 降参確認画面を出すボタンの処理
    data.isHoverIdx[BattleOption::RETURN] = input.IsMouseOver(RET_BUT_X, RET_BUT_Y, RET_BUTW, RET_BUTH);
    if (input.IsLeftClicked() && data.isHoverIdx[BattleOption::RETURN]) {
        data.isSurrenderConfirm = true;
    }
}

// -------------------------------------------------------------
// 攻撃・防御の決定ボタン処理
// -------------------------------------------------------------
void BattleInputManager::ProcessActionButtons(BattleData& data, const InputManager& input, 
    int humanIdx, bool isHumanTurn, PlayerAction& action) {
    // マジックナンバー回避用変数
    const int DECISION_AREA_W = 271, DECISION_AREA_H = 325;
    const int ATK_BTN_X = 5, ATK_BTN_Y = 60, DEF_BTN_X = 340, DEF_BTN_Y = 60;

    Player& turnPlayer = data.Player_Turn[data.currentTurnIdx];
    const auto& turnHandVec = turnPlayer.Hand.GetCards();

    // 【攻撃フェーズ】
    if (data.currentPhase == BattlePhase::Select && !data.selectedCards.empty() && isHumanTurn) {
        data.isHoverIdx[BattleOption::ATTACK] = input.IsMouseOver(ATK_BTN_X, ATK_BTN_Y, DECISION_AREA_W, DECISION_AREA_H);

        if (input.IsLeftClicked() && data.isHoverIdx[BattleOption::ATTACK]) {
            data.selectedOption = BattleOption::ATTACK;

            // 使用するカードのカテゴリ取得を、オートターゲット処理の「外」に出す
            CardCategory firstCardCat = Attack;
            if (!data.selectedCards.empty() && data.selectedCards[0] < (int)turnHandVec.size()) {
                firstCardCat = turnHandVec[data.selectedCards[0]].GetCategory();
            }

            // 回復系カード判定を行い、行動データにセットする
            if (firstCardCat == Healing || firstCardCat == MagicHealing) {
                action.isHealAction = true;
            }

            // オートターゲット処理
            if (!data.playerTarget || data.targetIdx == -1) {
                if (firstCardCat == Healing || firstCardCat == MagicHealing) {
                    data.targetIdx = data.currentTurnIdx;
                }
                else {
                    std::vector<int> aliveEnemies;
                    for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
                        if (i != data.currentTurnIdx && !data.Player_Turn[i].Status.dead) {
                            aliveEnemies.push_back(i);
                        }
                    }
                    data.targetIdx = aliveEnemies.empty() ? data.currentTurnIdx : aliveEnemies[rand() % aliveEnemies.size()];
                }
                data.playerTarget = true;
            }

            // 勝手にフェーズを変えるのをやめ、行動を通知するだけにする(変更)
            action.isAttackDecision = true;
            action.hasAction = true;
        }
    }
    // 【防御フェーズ】
    else if (data.currentPhase == BattlePhase::DefenseSelect && data.targetIdx == humanIdx) {
        data.isHoverIdx[BattleOption::DEFENSE] = input.IsMouseOver(DEF_BTN_X, DEF_BTN_Y, DECISION_AREA_W, DECISION_AREA_H);

        if (input.IsLeftClicked() && data.isHoverIdx[BattleOption::DEFENSE]) {
            // 勝手にフェーズを変えるのをやめ、行動を通知するだけにする(こちらも変更)
            action.isDefenseDecision = true;
            action.hasAction = true;
        }
    }
}

// -------------------------------------------------------------
// マニュアルターゲット選択処理
// -------------------------------------------------------------
void BattleInputManager::ProcessTargetSelection(BattleData& data, const InputManager& input, bool isHumanTurn) {
    if (data.currentPhase == BattlePhase::Select && isHumanTurn) {
        const int STATUS_START_X = 700;
        const int STATUS_START_Y = 75;
        const int STATUS_MARGIN_Y = 40;
        const int STATUS_WIDTH = 275;
        const int STATUS_HEIGHT = 30;

        for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
            int currentY = STATUS_START_Y + i * STATUS_MARGIN_Y;
            data.isHoverPlayerIdx[i] = input.IsMouseOver(STATUS_START_X, currentY - 15, STATUS_WIDTH, STATUS_HEIGHT);

            if (input.IsLeftClicked() && data.isHoverPlayerIdx[i]) {
                data.targetIdx = i;
                data.playerTarget = true;
            }
        }
    }
}

// -------------------------------------------------------------
// 手札の選択・コンボ処理
// -------------------------------------------------------------
void BattleInputManager::ProcessHandSelection(BattleData& data, const InputManager& input, Player& humanPlayer, int humanIdx, bool isHumanTurn) {
    const float SCALE = 1.45f;
    const int CARD_W = (int)(CARD_CELL * SCALE);
    const int CARD_H = (int)(CARD_CELL * SCALE);
    const int HAND_START_X = 10;
    const int HAND_START_Y = 450;
    const int MARGIN = 2;
    const int MAX_CARDS_PER_ROW = 9;
    const int ROW_SPACING = CARD_H + 30;

    const auto& humanHandVec = humanPlayer.Hand.GetCards();

    for (int i = 0; i < (int)humanHandVec.size(); ++i) {
        int col = i % MAX_CARDS_PER_ROW;
        int row = i / MAX_CARDS_PER_ROW;
        int x = HAND_START_X + (CARD_W + MARGIN) * col;
        int y = HAND_START_Y + (ROW_SPACING * row);

        if (input.IsMouseOver(x, y, CARD_W, CARD_H + 25)) {
            data.hoveredCardIdx = i;
        }

        // 選択可能判定
        bool isSelectable = false;
        if (data.currentPhase == BattlePhase::Select && isHumanTurn) {
            isSelectable = (humanHandVec[i].GetCategory() != Defense);
        }
        else if (data.currentPhase == BattlePhase::DefenseSelect && data.targetIdx == humanIdx) {
            int cat = humanHandVec[i].GetCategory();
            isSelectable = (cat == Defense || cat == Bilingual);
        }

        // まず、現在選択中かどうかを確認する変数を作る
        std::vector<int>& activeSelection = (data.currentPhase == BattlePhase::Select) ? data.selectedCards : data.selectedDefenseCards;
        bool isAlreadySelected = (std::find(activeSelection.begin(), activeSelection.end(), i) != activeSelection.end());

        // クリックの可否判定
        // ・すでに選ばれているなら：解除できるので true
        // ・選ばれていないなら：Logic側の判定に従う
        bool canClick = isAlreadySelected || (i < (int)data.isCardSelectable.size() && data.isCardSelectable[i]);

        // ゲート：クリック判定に入るための最低条件
        if (canClick && input.IsMouseOver(x, y, CARD_W, CARD_H)) {
            data.isHoverCardIdx[i] = true;

            if (input.IsLeftClicked()) {
                auto it = std::find(activeSelection.begin(), activeSelection.end(), i);

                // --- 解除処理 (すでに選ばれているなら isSelectable に関係なく許可) ---
                if (it != activeSelection.end()) {
                    if (it == activeSelection.begin()) {
                        activeSelection.clear(); // ベース解除
                    }
                    else {
                        activeSelection.erase(it); // 単体解除
                    }
                    // 解除時は再計算へ進む
                }
                // --- 新規選択処理 (isSelectable が true の時のみ許可) ---
                else if (isSelectable) {
                    bool isClickedAddable = humanHandVec[i].GetAdd();
                    CardCategory clickedCat = humanHandVec[i].GetCategory();
                    bool isClickedHeal = (clickedCat == Healing || clickedCat == MagicHealing);

                    if (activeSelection.empty()) {
                        activeSelection.push_back(i);
                    }
                    else {
                        int baseIdx = activeSelection[0];
                        CardCategory baseCat = humanHandVec[baseIdx].GetCategory();
                        bool isBaseHeal = (baseCat == Healing || baseCat == MagicHealing);  // 回復系カードかの判定
                        bool isClickedBilingual = (clickedCat == Bilingual);
                        bool isBaseForbiddenToAdd = (baseCat == Magic || baseCat == All || isBaseHeal); // 加算禁止カードの判定増加(全体攻撃カードと奇跡カード)

                        // 上書き判定
                        // 1枚目が「加算禁止」または「特定の条件」の場合は、強制的にクリアして新規選択にする
                        if (isBaseForbiddenToAdd || !isClickedAddable || isClickedHeal || (data.currentPhase == BattlePhase::Select && isClickedBilingual)) {
                            activeSelection.clear();
                            activeSelection.push_back(i);
                        }
                        // 追加許可判定
                        // 1枚目が「売カード」または「加算禁止ではないカテゴリ」の場合のみ、追加を許可する
                        else if (baseCat == Sell || !isBaseForbiddenToAdd) {
                            activeSelection.push_back(i);
                        }
                        else {
                            // 選択ルールに合致しない場合（ここに入ると何も追加されない）
                            continue;
                        }
                    }
                }
                else {
                    // isSelectable が false で、かつ選択済みでもない場合は何もしない
                    continue;
                }

                // --- 属性と威力の再計算 (選択状態が変わったときのみ実行) ---
                BattleLogicManager logic;
                if (data.currentPhase == BattlePhase::Select) {
                    logic.RecalculateAttackElement(data, humanHandVec);
                    data.attackTotalPower = 0;
                    for (int idx : activeSelection) data.attackTotalPower += humanHandVec[idx].GetPower();
                }
                else if (data.currentPhase == BattlePhase::DefenseSelect && data.targetIdx == humanIdx) {
                    logic.RecalculateDefenseElement(data, humanHandVec);
                    data.defenseTotalPower = 0;
                    for (int idx : activeSelection) data.defenseTotalPower += humanHandVec[idx].GetPower();
                }
            }
        }
    }
}