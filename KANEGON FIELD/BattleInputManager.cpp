#include "BattleInputManager.h"
#include "BattleData.h"
#include "BattleLogicManager.h"
#include "Card.h"
#include "Player.h"
#include <algorithm>

bool BattleInputManager::Update(BattleData& data, const InputManager& input, Player& humanPlayer, int humanIdx, bool isHumanTurn) {
	// =============================================================
	// プレイヤーの入力処理（UI・ボタン類）
	// =============================================================
	
	// マウスホバー状態の初期化
	for (int i = 0; i < BattleOption::MAX; i++) {
		data.isHoverIdx[i] = false;
	}

	// -------------------------------------------------------------
	// 【降参（サレンダー）確認画面の処理】
	// -------------------------------------------------------------
	if (data.isSurrenderConfirm) {
		// ボタンの座標等の変数
		int give_butX = 425; int give_butY = 300; int give_w = 150; int give_h = 50;
		
		// マウス判定の追加
		data.isHoverIdx[BattleOption::GIVE_UP] = input.IsMouseOver(give_butX, give_butY, give_w, give_h);

		// クリック判定
		bool clickedReturnAgain = (input.IsLeftClicked() && input.IsMouseOver(RET_BUT_X, RET_BUT_Y, RET_BUTW, RET_BUTH));
		bool clickedOutside = (input.IsLeftClicked() && !input.IsMouseOver(300, 200, 400, 200));

		// 降参キャンセル
		if (clickedReturnAgain || clickedOutside) {
			data.isSurrenderConfirm = false;
		}
		else if (input.IsLeftClicked() && data.isHoverIdx[BattleOption::GIVE_UP]) {
			// 降参決定したら
			data.selectedCards.clear();
			data.selectedDefenseCards.clear();
			data.playerTarget = false;
			data.targetIdx = -1;
			data.totalPower = 0;
			data.isSurrenderConfirm = false;
			data.selectedOption = BattleOption::RETURN;
			return true;
		}
		return false;
	}

	// -------------------------------------------------------------
	// 【戻る（降参）ボタンの処理】
	// -------------------------------------------------------------
	data.isHoverIdx[BattleOption::RETURN] = input.IsMouseOver(RET_BUT_X, RET_BUT_Y, RET_BUTW, RET_BUTH);
	if (input.IsLeftClicked() && data.isHoverIdx[BattleOption::RETURN]) {
		data.isSurrenderConfirm = true;
	}

	Player& turnPlayer = data.Player_Turn[data.currentTurnIdx];

	// 攻撃・防御決定ボタン用座標変数
	const int DECISION_AREA_W = 150;
	const int DECISION_AREA_H = 40;
	const int ATK_BTN_X = 250;
	const int ATK_BTN_Y = 150;
	const int DEF_BTN_X = 250; 
	const int DEF_BTN_Y = 220;

	// ターンプレイヤーの手札ベクターを取得
	const auto& turnHandVec = turnPlayer.Hand.GetCards();

	// -------------------------------------------------------------
	// 【攻撃フェーズ時の決定ボタン処理】
	// -------------------------------------------------------------
	// 【条件】攻撃選択フェーズ ＆ カードが1枚以上選ばれている ＆ 自分のターン
	if (data.currentPhase == BattlePhase::Select && !data.selectedCards.empty() && isHumanTurn) {
		data.isHoverIdx[BattleOption::ATTACK] = input.IsMouseOver(ATK_BTN_X, ATK_BTN_Y, DECISION_AREA_W, DECISION_AREA_H);

		if (input.IsLeftClicked() && data.isHoverIdx[BattleOption::ATTACK]) {
			data.selectedOption = BattleOption::ATTACK;

			// オートターゲット機能
			if (!data.playerTarget || data.targetIdx == -1) {
				CardCategory firstCardCat = Attack;
				if (!data.selectedCards.empty() && data.selectedCards[0] < (int)turnHandVec.size()) {
					firstCardCat = turnHandVec[data.selectedCards[0]].GetCategory();
				}

				bool isHeal = (firstCardCat == Healing || firstCardCat == MagicHealing);
				if (isHeal) {
					// 回復カードなら自動的に自分をターゲットにする
					data.targetIdx = data.currentTurnIdx;
				}
				else {
					// 攻撃カードなら「生存している敵」からランダムにターゲットを決定
					std::vector<int> aliveEnemies;
					for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
						// 生存判定
						if (i != data.currentTurnIdx && !data.Player_Turn[i].Status.dead) {
							aliveEnemies.push_back(i);
						}
					}
					if (!aliveEnemies.empty()) {
						data.targetIdx = aliveEnemies[rand() % aliveEnemies.size()];
					}
					else {
						// 敵がいない場合
						data.targetIdx = data.currentTurnIdx;
					}
				}
				// 攻撃確定後、防御側の防御フェーズに移行
				data.playerTarget = true;
			}

			data.currentPhase = BattlePhase::DefenseSelect;
			return false;
		}
	}

	// -------------------------------------------------------------
	// 【防御フェーズ時の決定ボタン処理】
	// -------------------------------------------------------------
	// 【条件】防御選択フェーズ ＆ 自分が攻撃のターゲットにされている
	else if (data.currentPhase == BattlePhase::DefenseSelect && data.targetIdx == humanIdx) {
		data.isHoverIdx[BattleOption::ATTACK] = input.IsMouseOver(DEF_BTN_X, DEF_BTN_Y, DECISION_AREA_W, DECISION_AREA_H);

		if (input.IsLeftClicked() && data.isHoverIdx[BattleOption::ATTACK]) {
			// 防御が確定したので、カードオープンフェーズへ移行
			data.currentPhase = BattlePhase::Reveal;
			data.revealIndex = 0;			// 演出用のインデックス初期化
			data.animationTimer = 15;		// 演出用のタイマー初期化
			return false;
		}
	}

	// =============================================================
	// ターゲット選択判定（ステータスUIのクリック）
	// =============================================================
	// 自分のターンの攻撃選択フェーズのみ、手動でのターゲット切り替えを許可
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

	// =============================================================
	// 手札のカード選択判定
	// =============================================================
	data.hoveredCardIdx = -1;

	const float SCALE = 1.45f;
	const int CARD_W = (int)(CARD_CELL * SCALE);
	const int CARD_H = (int)(CARD_CELL * SCALE);

	const int HAND_START_X = 10;
	const int HAND_START_Y = 450;
	const int MARGIN = 2;
	const int MAX_CARDS_PER_ROW = 9;
	const int ROW_SPACING = CARD_H + 30;

	// 操作プレイヤーの手札ベクターを取得
	const auto& humanHandVec = humanPlayer.Hand.GetCards();

	for (int i = 0; i < (int)humanHandVec.size(); ++i) {
		// カードの描画座標を計算(グリッド配置)
		int col = i % MAX_CARDS_PER_ROW;
		int row = i / MAX_CARDS_PER_ROW;
		int x = HAND_START_X + (CARD_W + MARGIN) * col;
		int y = HAND_START_Y + (ROW_SPACING * row);

		// マウスホバー判定
		if (input.IsMouseOver(x, y, CARD_W, CARD_H + 25)) {
			data.hoveredCardIdx = i;
		}

		// -------------------------------------------------------------
		// カードが選択可能かどうかの判定 (フェーズとカード種類による制限)
		// -------------------------------------------------------------
		bool isSelectable = true;

		if (data.currentPhase == BattlePhase::Select) {
			if (isHumanTurn) {
				// 攻撃フェーズ中は防御専用カードは選択不可
				int cat = humanHandVec[i].GetCategory();
				if (cat == Defense) isSelectable = false;
			}
			else {
				// 相手のターンの攻撃フェーズは選択不可
				isSelectable = false;
			}
		}
		else if (data.currentPhase == BattlePhase::DefenseSelect) {
			if (data.targetIdx == humanIdx) {
				// 防御フェーズ中は防具系カードのみ選択可能
				int cat = humanHandVec[i].GetCategory();
				if (cat != Defense && cat != Bilingual) isSelectable = false;
			}
			else {
				// 他人が攻撃されている時は選択不可
				isSelectable = false;
			}
		}
		else {
			// その他フェーズ中も選択不可
			isSelectable = false;
		}

		// -------------------------------------------------------------
		// カードがクリックされた時の処理（選択・解除・コンボ判定）
		// -------------------------------------------------------------
		if (isSelectable) {
			data.isHoverCardIdx[i] = input.IsMouseOver(x, y, CARD_W, CARD_H);

			if (input.IsLeftClicked() && data.isHoverCardIdx[i]) {
				// 加算可能か判定
				bool isClickedAddable = humanHandVec[i].GetAdd();
				CardCategory clickedCat = humanHandVec[i].GetCategory();
				bool isClickedHeal = (clickedCat == Healing || clickedCat == MagicHealing);

				std::vector<int>& activeSelection = (data.currentPhase == BattlePhase::Select) ? data.selectedCards : data.selectedDefenseCards;

				auto it = std::find(activeSelection.begin(), activeSelection.end(), i);

				if (it != activeSelection.end()) {
					if (it == activeSelection.begin()) {
						activeSelection.clear();
						if (data.currentPhase == BattlePhase::Select) data.currentAttackElement = "無";
					}
					else {
						activeSelection.erase(it);
						if (data.currentPhase == BattlePhase::Select) data.currentAttackElement = "再計算が必要";
					}
				}
				else {
					if (activeSelection.empty()) {
						activeSelection.push_back(i);
						if (data.currentPhase == BattlePhase::Select) {
							std::string baseType = humanHandVec[i].GetType();
							data.currentAttackElement = (baseType == "") ? "無" : baseType;
						}
					}
					else {
						int baseIdx = activeSelection[0];
						const auto& baseCard = humanHandVec[baseIdx];

						CardCategory baseCat = baseCard.GetCategory();
						bool isBaseHeal = (baseCat == Healing || baseCat == MagicHealing);
						bool isClickedBilingual = (clickedCat == Bilingual);

						if (!isClickedAddable || isClickedHeal || (data.currentPhase == BattlePhase::Select && isClickedBilingual)) {
							activeSelection.clear();
							activeSelection.push_back(i);
							if (data.currentPhase == BattlePhase::Select) {
								std::string baseType = humanHandVec[i].GetType();
								data.currentAttackElement = (baseType == "") ? "無" : baseType;
							}
						}
						else {
							if (baseCat == All || isBaseHeal) {
								// 何もしない
							}
							else {
								activeSelection.push_back(i);
								if (data.currentPhase == BattlePhase::Select) {
									data.currentAttackElement = "再計算が必要";
								}
							}
						}
					}
				}

				// -------------------------------------------------------------
				// 最終的な威力の再計算
				// -------------------------------------------------------------
				data.totalPower = 0;
				for (int idx : activeSelection) {
					if (idx >= 0 && idx < (int)humanHandVec.size()) {
						data.totalPower += humanHandVec[idx].GetPower();
					}
				}
			}
		}
	}
	return false;
}