#include "BattleAIManager.h"
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

	// =============================================================
	// 攻撃フェーズ（AIのターン時）
	// =============================================================
	if (data.currentPhase == BattlePhase::Select && !isHumanTurn) {

		data.selectedCards.clear();

		int currentMp = turnPlayer.getMp();
		int totalPower = 0;

		int bestIndex = -1;
		int maxPower = -1;

		// --- 手札から【MPが足りる】かつ【一番強い】武器や魔法を選ぶ ---
		for (int i = 0; i < (int)hand.size(); ++i) {
			CardCategory cat = hand[i].GetCategory();
			// 防御・回復系はベースにしない
			if (cat != Defense && cat != Healing && cat != MagicHealing && hand[i].GetMP() <= currentMp) {
				// より攻撃力が高いカードを記憶する
				if (hand[i].GetPower() > maxPower) {
					maxPower = hand[i].GetPower();
					bestIndex = i;
				}
			}
		}

		// --- 攻撃カードが選べた場合 ---
		if (bestIndex != -1) {
			data.selectedCards.push_back(bestIndex);
			currentMp -= hand[bestIndex].GetMP();
			totalPower += hand[bestIndex].GetPower();

			// 加算カード選択
			for (int i = 0; i < (int)hand.size(); ++i) {
				if (i == bestIndex) continue;

				CardCategory cat = hand[i].GetCategory();
				// 加算可能なカテゴリ（Attack, Magic のみ）に限定する
				// 攻防(Bilingual)、全体(All)、回復(Healing, MagicHealing)、防御(Defense)は加算させない
				bool isAddableCategory = (cat == Attack || cat == Magic);

				if (isAddableCategory && hand[i].GetAdd() && hand[i].GetMP() <= currentMp) {
					data.selectedCards.push_back(i);
					currentMp -= hand[i].GetMP();
					totalPower += hand[i].GetPower();

					// 重ね掛けで攻撃の属性を変える仕様の場合、ここで属性を上書きします
					data.currentAttackElement = hand[i].GetType();
				}
			}

			data.attackTotalPower = totalPower;

			// 生きている敵（自分以外）からランダムにターゲットを選ぶ
			std::vector<int> aliveEnemies;
			for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
				// Status.dead を参照して気絶判定
				if (i != data.currentTurnIdx && !data.Player_Turn[i].Status.dead) {
					aliveEnemies.push_back(i);
				}
			}

			if (!aliveEnemies.empty()) {
				// 乱数のタネ（ハードウェアのノイズなどから生成）
				std::random_device rd;

				// メルセンヌ・ツイスタ（超高品質な乱数生成エンジン）
				std::mt19937 gen(rd());

				// 範囲の指定（0 ～ 生きている敵の数-1 の間で「完全に均等な」確率を作る）
				std::uniform_int_distribution<int> dist(0, aliveEnemies.size() - 1);

				// 実際に乱数を発生させてターゲットを決める
				data.targetIdx = aliveEnemies[dist(gen)];
			}
			else {
				data.targetIdx = data.currentTurnIdx; // フェールセーフ
			}
			data.playerTarget = true;

			// 攻撃先が決まったら防御選択フェーズへ
			data.currentPhase = BattlePhase::DefenseSelect;
		}
		else {
			// 攻撃できない場合はターン終了処理へ
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
			int totalDefensePower = 0; // 合計防御力用の変数を追加

			// AI（防御側）の手札を取得する
			const auto& targetHand = targetPlayer.Hand.GetCards();

			// 属性で守れるカードを探してコンボにする
			for (int i = 0; i < (int)targetHand.size(); ++i) {
				CardCategory cat = targetHand[i].GetCategory();

				// 守れるカテゴリのみ
				if ((cat == Defense || cat == Bilingual) && targetHand[i].GetMP() <= currentMp) {

					// 防御属性が合致する場合のみ追加
					if (DamageResolver::IsValidGuard(incomingElement, targetHand[i].GetType())) {
						data.selectedDefenseCards.push_back(i);
						currentMp -= targetHand[i].GetMP();
						totalDefensePower += targetHand[i].GetPower(); // 防御力を加算
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