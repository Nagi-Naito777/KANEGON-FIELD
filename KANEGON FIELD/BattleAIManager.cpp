#include "BattleAIManager.h"
#include "BattleLogicManager.h"
#include "BattleData.h"
#include "Card.h"
#include "Player.h"
#include "DamageResult.h"
#include <vector>
#include <random>

// シグネチャから不要な引数（humanIdx, isHumanTurn）を削除。内部でControllerTypeを判定します。
void BattleAIManager::Update(BattleData& data) {

	// =============================================================
	// 攻撃フェーズ（ターンのプレイヤーがAIか判定）
	// =============================================================
	if (data.currentPhase == BattlePhase::Select) {

		Player& turnPlayer = data.Player_Turn[data.currentTurnIdx];
		// ターンの持ち主が人間ならAIは何もしない
		if (turnPlayer.getControllerType() == ControllerType::HUMAN) return;

		BattleLogicManager logic;

		// --- 確定用データのリセット ---
		data.confirmedAttackCards.clear();

		int currentMp = turnPlayer.getMp();
		int bestAttackIndex = -1, maxAttackPower = -1;
		int bestHealIndex = -1, maxHealPower = -1;

		const auto& hand = turnPlayer.Hand.GetCards();

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
				// 回復なので防御フェーズをスキップして公開へ
				data.currentPhase = BattlePhase::DefenseReveal;
			}
			else {
				// 攻撃の場合のコンボロジック
				CardCategory baseCat = hand[bestIndex].GetCategory();
				data.currentAttackElement = hand[bestIndex].GetType();

				if (baseCat != All) {
					for (int i = 0; i < (int)hand.size(); ++i) {
						if (i == bestIndex) continue;

						CardCategory cat = hand[i].GetCategory();
						// ★修正：isCardSelectable(UI側データ)を使わず、純粋なルールで追加判定を行う
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
			// 何も出せない場合
			data.currentTurnIdx = (data.currentTurnIdx + 1) % data.Player_Turn.size();
		}
	}

	// =============================================================
	// 防御フェーズ（ターゲットがAIか判定）
	// =============================================================
	else if (data.currentPhase == BattlePhase::DefenseSelect) {

		// ターゲットが有効範囲内かチェック
		if (data.targetIdx >= 0 && data.targetIdx < (int)data.Player_Turn.size()) {

			Player& targetPlayer = data.Player_Turn[data.targetIdx];

			// ターゲットが人間ならAIは操作しない
			if (targetPlayer.getControllerType() == ControllerType::HUMAN) return;

			BattleLogicManager logic;
			data.confirmedDefenseCards.clear();

			int currentMp = targetPlayer.getMp();
			std::string incomingElement = data.currentAttackElement;
			int incomingAttackPower = data.attackTotalPower;
			int totalDefensePower = 0;

			const auto& targetHand = targetPlayer.Hand.GetCards();

			// 属性で守れるカードを探してコンボにする
			for (int i = 0; i < (int)targetHand.size(); ++i) {
				CardCategory cat = targetHand[i].GetCategory();

				if ((cat == Defense || cat == Bilingual) && targetHand[i].GetMP() <= currentMp) {

					if (logic.CanSelectDefenseCard(data, data.confirmedDefenseCards, targetPlayer, i, incomingElement)) {
						data.confirmedDefenseCards.push_back(i);
						currentMp -= targetHand[i].GetMP();
						totalDefensePower += targetHand[i].GetPower(); // 防御力を加算

						// 相手の攻撃力を上回ったら、それ以上防御カードの無駄遣いをしない
						if (totalDefensePower >= incomingAttackPower) {
							break;
						}
					}
				}
			}

			data.defenseTotalPower = totalDefensePower;
			data.currentPhase = BattlePhase::DefenseReveal;
		}
	}
}