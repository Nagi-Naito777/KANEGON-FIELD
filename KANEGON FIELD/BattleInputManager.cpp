#include "BattleInputManager.h"
#include "BattleData.h"
#include "BattleLogicManager.h"
#include "Card.h"
#include "Player.h"
#include <algorithm>
#include <vector>

// 更新処理
bool BattleInputManager::Update(BattleData& data, const InputManager& input, Player& humanPlayer, int humanIdx, bool isHumanTurn) {
    // 毎フレームの初期化
    std::fill(std::begin(data.isHoverIdx), std::end(data.isHoverIdx), false);
    std::fill(std::begin(data.isHoverPlayerIdx), std::end(data.isHoverPlayerIdx), false);
    std::fill(std::begin(data.isHoverCardIdx), std::end(data.isHoverCardIdx), false);
    data.hoveredCardIdx = -1;

    // 降参画面が開いていたかを記憶する
    bool wasSurrenderConfirm = data.isSurrenderConfirm;

    // 降参UIの処理（降参が確定したら即座に true を返して終了）
    if (ProcessSurrender(data, input)) {
        return true;
    }

    // 元々開いていた(閉じた瞬間)または今開いた瞬間なら、他の判定をスキップ
    if (wasSurrenderConfirm || data.isSurrenderConfirm) {
        return false;
    }

    // 攻撃/防御の決定ボタン処理
    ProcessActionButtons(data, input, humanIdx, isHumanTurn);

    // ターゲットの手動選択処理
    ProcessTargetSelection(data, input, isHumanTurn);

    // 手札の選択・コンボ処理
    ProcessHandSelection(data, input, humanPlayer, humanIdx, isHumanTurn);

    // 通常通り戦闘継続
    return false;
}

// -------------------------------------------------------------
// 降参UIの処理
// -------------------------------------------------------------
bool BattleInputManager::ProcessSurrender(BattleData& data, const InputManager& input) {
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
            data.isSurrenderConfirm = false; // 降参キャンセル
        }
        else if (input.IsLeftClicked() && data.isHoverIdx[BattleOption::GIVE_UP]) {
            // 降参決定時のデータリセット
            data.selectedCards.clear();
            data.selectedDefenseCards.clear();
            data.playerTarget = false;
            data.targetIdx = -1;
            data.attackTotalPower = 0;
            data.defenseTotalPower = 0;
            data.isSurrenderConfirm = false;
            data.selectedOption = BattleOption::RETURN;
            return true; // 降参によるバトルループ終了
        }
        return false;
    }

    // 降参確認画面を出すボタンの処理
    data.isHoverIdx[BattleOption::RETURN] = input.IsMouseOver(RET_BUT_X, RET_BUT_Y, RET_BUTW, RET_BUTH);
    if (input.IsLeftClicked() && data.isHoverIdx[BattleOption::RETURN]) {
        data.isSurrenderConfirm = true;
    }

    return false;
}

// -------------------------------------------------------------
// 攻撃・防御の決定ボタン処理
// -------------------------------------------------------------
void BattleInputManager::ProcessActionButtons(BattleData& data, const InputManager& input, int humanIdx, bool isHumanTurn) {
    const int DECISION_AREA_W = 271;
    const int DECISION_AREA_H = 325;
    const int ATK_BTN_X = 5;
    const int ATK_BTN_Y = 60;
    const int DEF_BTN_X = 340;
    const int DEF_BTN_Y = ATK_BTN_Y;

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

            // オートターゲット機能
            if (!data.playerTarget || data.targetIdx == -1) {
                if (firstCardCat == Healing || firstCardCat == MagicHealing) {
                    data.targetIdx = data.currentTurnIdx; // 回復は自分
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

            // 回復系なら防御選択フェーズをスキップして、即座に演出フェーズへ移行する
            if (firstCardCat == Healing || firstCardCat == MagicHealing) {
                data.currentPhase = BattlePhase::DefenseReveal;
                data.revealIndex = 0;
                data.animDefenseCardCount = 0;
                data.animationTimer = 0;
            }
            else {
                // 攻撃など、相手が防ぐ必要があるものは通常の防御フェーズへ
                data.currentPhase = BattlePhase::DefenseSelect;
            }
        }
    }
    // 【防御フェーズ】
    else if (data.currentPhase == BattlePhase::DefenseSelect && data.targetIdx == humanIdx) {
        data.isHoverIdx[BattleOption::DEFENSE] = input.IsMouseOver(DEF_BTN_X, DEF_BTN_Y, DECISION_AREA_W, DECISION_AREA_H);

        if (input.IsLeftClicked() && data.isHoverIdx[BattleOption::DEFENSE]) {
            // ここで次の「演出フェーズ」へ移行する
            data.currentPhase = BattlePhase::DefenseReveal;
            data.revealIndex = 0;             // インデックスを0にリセット
            data.animDefenseCardCount = 0;    // カウントもリセット
            data.animationTimer = 0;          // 即座に開始
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

        // クリック処理
        if (isSelectable) {
            data.isHoverCardIdx[i] = input.IsMouseOver(x, y, CARD_W, CARD_H);

            if (input.IsLeftClicked() && data.isHoverCardIdx[i]) {
                bool isClickedAddable = humanHandVec[i].GetAdd();
                CardCategory clickedCat = humanHandVec[i].GetCategory();
                bool isClickedHeal = (clickedCat == Healing || clickedCat == MagicHealing);

                std::vector<int>& activeSelection = (data.currentPhase == BattlePhase::Select) ? data.selectedCards : data.selectedDefenseCards;
                auto it = std::find(activeSelection.begin(), activeSelection.end(), i);

                // 選択解除時の処理
                if (it != activeSelection.end()) {
                    if (it == activeSelection.begin()) {
                        // 1枚目（ベース）をクリックしたら全解除し、属性を初期化
                        activeSelection.clear();
                        if (data.currentPhase == BattlePhase::Select) {
                            data.currentAttackElement = "無";
                        }
                        else if (data.currentPhase == BattlePhase::DefenseSelect && data.targetIdx == humanIdx) {
                            data.currentDefenseElement = "無"; // 防御側の属性UI用
                        }
                    }
                    else {
                        // 2枚目以降ならそのカードだけ解除して属性再計算
                        activeSelection.erase(it);
                        BattleLogicManager logic;
                        if (data.currentPhase == BattlePhase::Select) {
                            logic.RecalculateAttackElement(data, humanHandVec);
                        }
                        else if (data.currentPhase == BattlePhase::DefenseSelect && data.targetIdx == humanIdx) {
                            logic.RecalculateDefenseElement(data, humanHandVec);
                        }
                    }
                }

                // 新規選択・コンボ追加時の処理
                else {
                    if (activeSelection.empty()) {
                        // 1枚目として選択
                        activeSelection.push_back(i);
                        std::string baseType = humanHandVec[i].GetType();

                        if (data.currentPhase == BattlePhase::Select) {
                            data.currentAttackElement = (baseType == "") ? "無" : baseType;
                        }
                        else if (data.currentPhase == BattlePhase::DefenseSelect && data.targetIdx == humanIdx) {
                            data.currentDefenseElement = (baseType == "") ? "無" : baseType;
                        }
                    }
                    else {
                        // 既にベースがある場合のコンボ判定
                        int baseIdx = activeSelection[0];
                        CardCategory baseCat = humanHandVec[baseIdx].GetCategory();
                        bool isBaseHeal = (baseCat == Healing || baseCat == MagicHealing);
                        bool isClickedBilingual = (clickedCat == Bilingual);

                        // 追加不可フラグ、回復カード、攻撃フェーズでの攻防カードならベースを上書き
                        if (!isClickedAddable || isClickedHeal || (data.currentPhase == BattlePhase::Select && isClickedBilingual)) {
                            activeSelection.clear();
                            activeSelection.push_back(i);
                            std::string baseType = humanHandVec[i].GetType();

                            if (data.currentPhase == BattlePhase::Select) {
                                data.currentAttackElement = (baseType == "") ? "無" : baseType;
                            }
                            else if (data.currentPhase == BattlePhase::DefenseSelect && data.targetIdx == humanIdx) {
                                data.currentDefenseElement = (baseType == "") ? "無" : baseType;
                            }
                        }
                        else {
                            // 全体攻撃や回復がベースの場合は追加を許可しない
                            if (baseCat == All || isBaseHeal) {
                                // 何もしない
                            }
                            else {
                                // 条件クリア：武器などの追加を許可し、属性を再計算
                                activeSelection.push_back(i);
                                BattleLogicManager logic;
                                if (data.currentPhase == BattlePhase::Select) {
                                    logic.RecalculateAttackElement(data, humanHandVec);
                                }
                                else if (data.currentPhase == BattlePhase::DefenseSelect && data.targetIdx == humanIdx) {
                                    logic.RecalculateDefenseElement(data, humanHandVec);
                                }
                            }
                        }
                    }
                }

                // 威力の再計算（ポインタを使ってフェーズごとに変数を切り替える）
                int* pTargetPower = nullptr;

                // フェーズに応じて操作対象の変数を決める
                if (data.currentPhase == BattlePhase::Select) {
                    pTargetPower = &data.attackTotalPower;
                }
                else if (data.currentPhase == BattlePhase::DefenseSelect && data.targetIdx == humanIdx) {
                    pTargetPower = &data.defenseTotalPower;
                }

                // ポインタが有効なら計算を実行
                if (pTargetPower != nullptr) {
                    *pTargetPower = 0; // 0にリセット

                    for (int idx : activeSelection) {
                        if (idx >= 0 && idx < (int)humanHandVec.size()) {
                            *pTargetPower += humanHandVec[idx].GetPower();
                        }
                    }
                }
            }
        }
    }
}