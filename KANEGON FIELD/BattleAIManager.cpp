#include "BattleAIManager.h"
#include "BattleLogicManager.h"
#include "BattleData.h"
#include "Card.h"
#include "Player.h"
#include "DamageResult.h"
#include <vector>
#include <random>

void BattleAIManager::Update(BattleData& data, int humanIdx, bool isHumanTurn) {
	// 人間のターン時のSelectフェーズならAIは行動しない
	if (isHumanTurn && data.currentPhase == BattlePhase::Select) return;

	Player& turnPlayer = data.Player_Turn[data.currentTurnIdx];
	const auto& hand = turnPlayer.Hand.GetCards();

	// 人間側と同じ判定ロジックを利用するためのインスタンス化
	BattleLogicManager logic;

	// =============================================================
	// 攻撃フェーズ（AIのターン時）
	// =============================================================
	if (data.currentPhase == BattlePhase::Select && !isHumanTurn) {

		// --- 確定用データのリセット ---
		data.confirmedAttackCards.clear();

		// 手札全体の選択可否を更新（防御専用カード等をここで一括除外する）
		logic.UpdateCardSelectability(data, turnPlayer, true);

		int currentMp = turnPlayer.getMp();
		int bestAttackIndex = -1, maxAttackPower = -1;
		int bestHealIndex = -1, maxHealPower = -1;

		// --- 候補の選定 ---
		for (int i = 0; i < (int)hand.size(); ++i) {
			if (hand[i].GetMP() > currentMp) continue;

			CardCategory cat = hand[i].GetCategory();
			if (cat == Healing || cat == MagicHealing) {
				if (hand[i].GetPower() > maxHealPower) {
					maxHealPower = hand[i].GetPower();
					bestHealIndex = i;
				}
			}
			else if (cat != Defense) {
				if (hand[i].GetPower() > maxAttackPower) {
					maxAttackPower = hand[i].GetPower();
					bestAttackIndex = i;
				}
			}
		}

		bool useHeal = false;
		int bestIndex = -1;

		// --- 行動の決定（回復か攻撃か） ---
		if (bestHealIndex != -1 && bestAttackIndex != -1) {
			useHeal = (rand() % 2 == 0);
			bestIndex = useHeal ? bestHealIndex : bestAttackIndex;
		}
		else if (bestHealIndex != -1) {
			useHeal = true; bestIndex = bestHealIndex;
		}
		else if (bestAttackIndex != -1) {
			useHeal = false; bestIndex = bestAttackIndex;
		}

		// --- 行動の確定 ---
		if (bestIndex != -1) {
			data.confirmedAttackCards.push_back(bestIndex);
			currentMp -= hand[bestIndex].GetMP();
			int totalPower = hand[bestIndex].GetPower();

			if (useHeal) {
				data.attackTotalPower = totalPower;
				data.targetIdx = data.currentTurnIdx;
				data.playerTarget = true;
				// フェーズ移行のみ行う（アニメーションリセットはUI側の管理に任せる）
				data.currentPhase = BattlePhase::DefenseReveal;
			}
			else {
				// 攻撃の場合のコンボロジック
				CardCategory baseCat = hand[bestIndex].GetCategory();
				data.currentAttackElement = hand[bestIndex].GetType();

				if (baseCat != All) {
					for (int i = 0; i < (int)hand.size(); ++i) {
						if (i == bestIndex) continue;
						if (!data.isCardSelectable[i]) continue;

						CardCategory cat = hand[i].GetCategory();
						if ((cat == Attack || cat == Magic) && hand[i].GetAdd() && hand[i].GetMP() <= currentMp) {

							std::vector<int> tempSelected = data.confirmedAttackCards;
							tempSelected.push_back(i);
							std::string nextElement = logic.GetCombinedElement(tempSelected, hand);

							if (data.currentAttackElement != "無" && nextElement == "無") continue;

							data.confirmedAttackCards.push_back(i);
							currentMp -= hand[i].GetMP();
							totalPower += hand[i].GetPower();
							data.currentAttackElement = nextElement;
						}
					}
				}
				data.attackTotalPower = totalPower;

				// ターゲット選定
				std::vector<int> aliveEnemies;
				for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
					if (i != data.currentTurnIdx && !data.Player_Turn[i].Status.dead) aliveEnemies.push_back(i);
				}
				data.targetIdx = !aliveEnemies.empty() ? aliveEnemies[rand() % aliveEnemies.size()] : data.currentTurnIdx;
				data.playerTarget = true;

				data.currentPhase = BattlePhase::DefenseSelect;
			}
		}
		else {
			data.currentTurnIdx = (data.currentTurnIdx + 1) % data.Player_Turn.size();
		}
	}

	// =============================================================
	// 防御フェーズ（AIがターゲットにされている時）
	// =============================================================
	else if (data.currentPhase == BattlePhase::DefenseSelect) {

		// 攻撃のターゲットが人間以外（AIが防御する番）かチェック
		if (data.targetIdx != humanIdx && data.targetIdx >= 0 && data.targetIdx < (int)data.Player_Turn.size()) {

			Player& targetPlayer = data.Player_Turn[data.targetIdx];
			data.confirmedDefenseCards.clear(); // 名前を統一

			int currentMp = targetPlayer.getMp();
			std::string incomingElement = data.currentAttackElement;
			int incomingAttackPower = data.attackTotalPower; // 相手の攻撃力を取得
			int totalDefensePower = 0; // 合計防御力用の変数を追加

			// AI（防御側）の手札を取得する
			const auto& targetHand = targetPlayer.Hand.GetCards();

			// 相手の攻撃に合わせて手札の選択可否を更新（属性相性不利などをここで弾く）
			logic.UpdateCardSelectability(data, targetPlayer, false);

			// 属性で守れるカードを探してコンボにする
			for (int i = 0; i < (int)targetHand.size(); ++i) {
				CardCategory cat = targetHand[i].GetCategory();

				// 守れるカテゴリのみ
				if ((cat == Defense || cat == Bilingual) && targetHand[i].GetMP() <= currentMp) {

					// UIロジックと同じ「コンボとして追加して防げるか」の厳密な判定を利用
					if (logic.CanSelectDefenseCard(data, targetPlayer, i, incomingElement)) {
						data.confirmedDefenseCards.push_back(i);
						currentMp -= targetHand[i].GetMP();
						totalDefensePower += targetHand[i].GetPower(); // 防御力を加算

						// ★追加：相手の攻撃力を上回ったら、それ以上防御カードの無駄遣いをしない
						if (totalDefensePower >= incomingAttackPower) {
							break;
						}
					}
				}
			}

			data.defenseTotalPower = totalDefensePower; // 計算した防御力をデータにセットする

			// 公開フェーズへ移行
			data.currentPhase = BattlePhase::DefenseReveal;
		}
	}
}