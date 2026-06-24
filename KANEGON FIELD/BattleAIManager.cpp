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

		data.selectedCards.clear();

		// 手札全体の選択可否を更新（防御専用カード等をここで一括除外する）
		logic.UpdateCardSelectability(data, turnPlayer, true);

		int currentMp = turnPlayer.getMp();
		// 攻撃用と回復用のベストなカードを記録する変数
		int bestAttackIndex = -1;	// 攻撃用
		int maxAttackPower = -1;	// 回復用

		int bestHealIndex = -1;
		int maxHealPower = -1;

		// --- 手札から【MPが足りる】かつ【一番強い】攻撃カードと回復カードを探す ---
		for (int i = 0; i < (int)hand.size(); ++i) {
			if (hand[i].GetMP() > currentMp) continue; // MP不足は除外

			CardCategory cat = hand[i].GetCategory();

			if (cat == Healing || cat == MagicHealing) {
				// 回復カードの候補
				if (hand[i].GetPower() > maxHealPower) {
					maxHealPower = hand[i].GetPower();
					bestHealIndex = i;
				}
			}
			else if (cat != Defense) {
				// 攻撃・魔法・全体などの攻撃系カードの候補
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
			// 両方持っている場合はランダムで決定
			// ※プレイヤーのHPを取得できるなら「if (HPが減っていたら) useHeal = true;」とするのがオススメです
			useHeal = (rand() % 2 == 0);
			bestIndex = useHeal ? bestHealIndex : bestAttackIndex;
		}
		else if (bestHealIndex != -1) {
			useHeal = true;
			bestIndex = bestHealIndex;
		}
		else if (bestAttackIndex != -1) {
			useHeal = false;
			bestIndex = bestAttackIndex;
		}

		// --- カードが選べた場合 ---
		if (bestIndex != -1) {
			data.selectedCards.push_back(bestIndex);
			currentMp -= hand[bestIndex].GetMP();
			int totalPower = hand[bestIndex].GetPower();

			if (useHeal) {
				// 【回復カードの場合】
				data.attackTotalPower = totalPower;   // 回復量としてセット
				data.targetIdx = data.currentTurnIdx; // ターゲットは自分自身に固定
				data.playerTarget = true;

				// 回復は防御フェーズが不要なので、即座に演出フェーズへ移行
				data.currentPhase = BattlePhase::DefenseReveal;
				data.revealIndex = 0;
				data.animDefenseCardCount = 0;
				data.animationTimer = 0;
			}
			else {
				// 【攻撃カードの場合】
				CardCategory baseCat = hand[bestIndex].GetCategory();

				// ベースカードの属性をセット
				data.currentAttackElement = hand[bestIndex].GetType();

				// ベースが全体攻撃（All）ではない場合のみ加算を許可する
				if (baseCat != All) {
					for (int i = 0; i < (int)hand.size(); ++i) {
						if (i == bestIndex) continue;
						if (!data.isCardSelectable[i]) continue; // 選択不可カードは除外

						CardCategory cat = hand[i].GetCategory();
						bool isAddableCategory = (cat == Attack || cat == Magic);

						if (isAddableCategory && hand[i].GetAdd() && hand[i].GetMP() <= currentMp) {
							// コンボした際の属性変化をシミュレーション
							std::vector<int> tempSelected = data.selectedCards;
							tempSelected.push_back(i);
							std::string nextElement = logic.GetCombinedElement(tempSelected, hand);

							// コンボによって有益な属性が「無」になってしまうならAIは追加を避ける
							if (data.currentAttackElement != "無" && nextElement == "無") {
								continue;
							}

							data.selectedCards.push_back(i);
							currentMp -= hand[i].GetMP();
							totalPower += hand[i].GetPower();

							// 共通ロジックから算出された正確な属性で更新
							data.currentAttackElement = nextElement;
						}
					}
				}

				data.attackTotalPower = totalPower;

				// 生きている敵（自分以外）からランダムにターゲットを選ぶ
				std::vector<int> aliveEnemies;
				for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
					if (i != data.currentTurnIdx && !data.Player_Turn[i].Status.dead) {
						aliveEnemies.push_back(i);
					}
				}

				if (!aliveEnemies.empty()) {
					std::random_device rd;
					std::mt19937 gen(rd());
					std::uniform_int_distribution<int> dist(0, aliveEnemies.size() - 1);
					data.targetIdx = aliveEnemies[dist(gen)];
				}
				else {
					data.targetIdx = data.currentTurnIdx; // フェールセーフ
				}
				data.playerTarget = true;

				// 攻撃先が決まったら防御選択フェーズへ
				data.currentPhase = BattlePhase::DefenseSelect;
			}
		}
		else {
			// 何もできない場合はターン終了処理へ
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
			data.selectedDefenseCards.clear();

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
						data.selectedDefenseCards.push_back(i);
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
			data.revealIndex = 0;
			data.animationTimer = 15;
		}
	}
}