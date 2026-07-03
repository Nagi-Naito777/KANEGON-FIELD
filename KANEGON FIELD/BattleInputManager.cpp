#include "BattleInputManager.h"
#include "BattleData.h"
#include "BattleLogicManager.h"
#include "Card.h"
#include "Player.h"
#include <algorithm>
#include <vector>

// 更新処理
PlayerAction BattleInputManager::Update(BattleData& data, LocalClientData& localData, const InputManager& input, Player& humanPlayer, int humanIdx, bool isHumanTurn) {
    PlayerAction action;

    // --- 【変更】毎フレームの初期化を localData に対して行う ---
    std::fill(std::begin(localData.isHoverIdx), std::end(localData.isHoverIdx), false);
    std::fill(std::begin(localData.isHoverPlayerIdx), std::end(localData.isHoverPlayerIdx), false);
    std::fill(std::begin(localData.isHoverCardIdx), std::end(localData.isHoverCardIdx), false);
    localData.hoveredCardIdx = -1;

    // --- 【変更】降参確認も localData を参照 ---
    bool wasSurrenderConfirm = localData.isSurrenderConfirm;

    ProcessSurrender(data, localData, input, action);
    if (action.isSurrender) return action;

    if (wasSurrenderConfirm || localData.isSurrenderConfirm) return action;

    // 攻撃/防御の決定ボタン処理
    ProcessActionButtons(data, localData, input, humanIdx, isHumanTurn, action);

    // ターゲットの手動選択処理
    ProcessTargetSelection(data, localData, input, isHumanTurn);

    // 手札の選択・コンボ処理
    ProcessHandSelection(data, localData, input, humanPlayer, humanIdx, isHumanTurn);

    return action;
}

// -------------------------------------------------------------
// 降参UIの処理
// -------------------------------------------------------------
void BattleInputManager::ProcessSurrender(BattleData& data, LocalClientData& localData, const InputManager& input, PlayerAction& action) {
    if (localData.isSurrenderConfirm) {
        // ギブアップのボタン描画用変数
        int give_butX = 425;
        int give_butY = 300;
        int give_w = 150;
        int give_h = 50;

        // マウス判定
        localData.isHoverIdx[BattleOption::GIVE_UP] = input.IsMouseOver(425, 300, 150, 50);

        bool clickedReturnAgain = (input.IsLeftClicked() && input.IsMouseOver(RET_BUT_X, RET_BUT_Y, RET_BUTW, RET_BUTH));
        bool clickedOutside = (input.IsLeftClicked() && !input.IsMouseOver(300, 200, 400, 200));

        if (clickedReturnAgain || clickedOutside) {
            localData.isSurrenderConfirm = false;
        }
        else if (input.IsLeftClicked() && localData.isHoverIdx[BattleOption::GIVE_UP]) {
            localData.isSurrenderConfirm = false;
            action.isSurrender = true;
            action.hasAction = true;
        }
        return;
    }

    // 降参確認画面を出すボタンの処理
    localData.isHoverIdx[BattleOption::RETURN] = input.IsMouseOver(RET_BUT_X, RET_BUT_Y, RET_BUTW, RET_BUTH);
    if (input.IsLeftClicked() && localData.isHoverIdx[BattleOption::RETURN]) {
        localData.isSurrenderConfirm = true;
    }
}

// -------------------------------------------------------------
// 攻撃・防御の決定ボタン処理
// -------------------------------------------------------------
void BattleInputManager::ProcessActionButtons(BattleData& data, 
    LocalClientData& localData, const InputManager& input,
    int humanIdx, bool isHumanTurn, PlayerAction& action) {
    // マジックナンバー回避用変数
    const int DECISION_AREA_W = 271, DECISION_AREA_H = 325;
    const int ATK_BTN_X = 5, ATK_BTN_Y = 60, DEF_BTN_X = 340, DEF_BTN_Y = 60;

    Player& turnPlayer = data.Player_Turn[data.currentTurnIdx];
    const auto& turnHandVec = turnPlayer.Hand.GetCards();

    // =============================================================
    // 【攻撃フェーズ】
    // =============================================================
    if (data.currentPhase == BattlePhase::Select && !localData.localSelectingCards.empty() && isHumanTurn) {
        // ホバー判定（localDataを使用）
        localData.isHoverIdx[BattleOption::ATTACK] = input.IsMouseOver(ATK_BTN_X, ATK_BTN_Y, DECISION_AREA_W, DECISION_AREA_H);

        if (input.IsLeftClicked() && localData.isHoverIdx[BattleOption::ATTACK]) {
            localData.selectedOption = BattleOption::ATTACK;

            // 1枚目に選ばれたカードのカテゴリで「回復」かどうかを判定
            int firstIdx = localData.localSelectingCards[0];
            CardCategory firstCardCat = turnHandVec[firstIdx].GetCategory();

            // 回復系アクションかどうか
            if (firstCardCat == Healing || firstCardCat == MagicHealing) {
                action.isHealAction = true;
            }

            // --- ターゲットの確定処理 ---
            // localData.localTargetIdx が選ばれていればそれを使い、未選択なら自動決定
            if (localData.localTargetIdx != -1) {
                data.targetIdx = localData.localTargetIdx;
            }
            else {
                if (action.isHealAction) {
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
            }
            data.playerTarget = true;

            // --- データを共有用（data）に確定させる ---
            data.confirmedAttackCards = localData.localSelectingCards;

            action.isAttackDecision = true;
            action.hasAction = true;
        }
    }
    // =============================================================
    // 【防御フェーズ】
    // =============================================================
    else if (data.currentPhase == BattlePhase::DefenseSelect && data.targetIdx == humanIdx) {
        // ホバー判定（localDataを使用）
        localData.isHoverIdx[BattleOption::DEFENSE] = input.IsMouseOver(DEF_BTN_X, DEF_BTN_Y, DECISION_AREA_W, DECISION_AREA_H);

        if (input.IsLeftClicked() && localData.isHoverIdx[BattleOption::DEFENSE]) {

            // 防御カードを確定させる
            data.confirmedDefenseCards = localData.localSelectingCards;

            action.isDefenseDecision = true;
            action.hasAction = true;
        }
    }
}

// -------------------------------------------------------------
// マニュアルターゲット選択処理
// -------------------------------------------------------------
void BattleInputManager::ProcessTargetSelection(BattleData& data, LocalClientData& localData, const InputManager& input, bool isHumanTurn) {
    if (data.currentPhase == BattlePhase::Select && isHumanTurn) {
        const int STATUS_START_X = 700;
        const int STATUS_START_Y = 75;
        const int STATUS_MARGIN_Y = 40;
        const int STATUS_WIDTH = 275;
        const int STATUS_HEIGHT = 30;

        for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
            int currentY = STATUS_START_Y + i * STATUS_MARGIN_Y;
            // localData を参照
            localData.isHoverPlayerIdx[i] = input.IsMouseOver(STATUS_START_X, currentY - 15, STATUS_WIDTH, STATUS_HEIGHT);

            if (input.IsLeftClicked() && localData.isHoverPlayerIdx[i]) {
                // ターゲットはまず localData に仮決定する
                localData.localTargetIdx = i;
            }
        }
    }
}

// -------------------------------------------------------------
// 手札の選択・コンボ処理
// -------------------------------------------------------------
void BattleInputManager::ProcessHandSelection(BattleData& data, LocalClientData& localData, const InputManager& input, Player& humanPlayer, int humanIdx, bool isHumanTurn) {
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
            localData.hoveredCardIdx = i;
        }

        // 選択可能判定（localDataを参照）
        bool isSelectable = false;
        if (data.currentPhase == BattlePhase::Select && isHumanTurn) {
            isSelectable = (humanHandVec[i].GetCategory() != Defense);
        }
        else if (data.currentPhase == BattlePhase::DefenseSelect && data.targetIdx == humanIdx) {
            int cat = humanHandVec[i].GetCategory();
            isSelectable = (cat == Defense || cat == Bilingual);
        }

        // 現在選択中のリストを localData から取得
        std::vector<int>& activeSelection = localData.localSelectingCards;
        bool isAlreadySelected = (std::find(activeSelection.begin(), activeSelection.end(), i) != activeSelection.end());

        // クリック可否判定（isCardSelectable も localData を参照）
        bool canClick = isAlreadySelected || (i < (int)localData.isCardSelectable.size() && localData.isCardSelectable[i]);

        if (canClick && input.IsMouseOver(x, y, CARD_W, CARD_H)) {
            localData.isHoverCardIdx[i] = true;

            if (input.IsLeftClicked()) {
                auto it = std::find(activeSelection.begin(), activeSelection.end(), i);

                // --- 解除処理 ---
                if (it != activeSelection.end()) {
                    if (it == activeSelection.begin()) activeSelection.clear();
                    else activeSelection.erase(it);
                }
                // --- 新規選択処理 ---
                else if (isSelectable) {
                    // ... (選択ロジックは既存と同じ)
                    bool isClickedAddable = humanHandVec[i].GetAdd();
                    CardCategory clickedCat = humanHandVec[i].GetCategory();
                    bool isClickedHeal = (clickedCat == Healing || clickedCat == MagicHealing);

                    if (activeSelection.empty()) {
                        activeSelection.push_back(i);
                    }
                    else {
                        int baseIdx = activeSelection[0];
                        CardCategory baseCat = humanHandVec[baseIdx].GetCategory();
                        bool isBaseHeal = (baseCat == Healing || baseCat == MagicHealing);
                        bool isClickedBilingual = (clickedCat == Bilingual);
                        bool isBaseForbiddenToAdd = (baseCat == Magic || baseCat == All || isBaseHeal);

                        if (isBaseForbiddenToAdd || !isClickedAddable || isClickedHeal || (data.currentPhase == BattlePhase::Select && isClickedBilingual)) {
                            activeSelection.clear();
                            activeSelection.push_back(i);
                        }
                        else if (baseCat == Sell || !isBaseForbiddenToAdd) {
                            activeSelection.push_back(i);
                        }
                        else { continue; }
                    }
                }
                else { continue; }

                // --- 属性と威力の再計算 ---
                // 【重要】LogicManager側で以下の変更が必要です：
                // RecalculateAttackElement(BattleData& data, const std::vector<int>& selection, ...);
                BattleLogicManager logic;
                if (data.currentPhase == BattlePhase::Select) {
                    // ここで localData.localSelectingCards を渡して計算させる
                    logic.RecalculateAttackElement(data, activeSelection, humanHandVec);
                    data.attackTotalPower = 0;
                    for (int idx : activeSelection) data.attackTotalPower += humanHandVec[idx].GetPower();
                }
                else if (data.currentPhase == BattlePhase::DefenseSelect && data.targetIdx == humanIdx) {
                    logic.RecalculateDefenseElement(data, activeSelection, humanHandVec);
                    data.defenseTotalPower = 0;
                    for (int idx : activeSelection) data.defenseTotalPower += humanHandVec[idx].GetPower();
                }
            }
        }
    }
}