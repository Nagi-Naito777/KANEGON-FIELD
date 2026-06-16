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
			// 防御カードはベースにしない
			if (cat != Defense && (cat == Attack || cat == Magic || cat == Bilingual) && hand[i].GetMP() <= currentMp) {
				// MPが足りているかも上のif文でチェック

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
				// 防御カードは絶対に追加しない。かつ加算可能カードのみ。
				bool isDefensive = (cat == Defense);
				if (!isDefensive && hand[i].GetAdd() && hand[i].GetMP() <= currentMp) {
					data.selectedCards.push_back(i);
					currentMp -= hand[i].GetMP();
					totalPower += hand[i].GetPower();
				}
			}

			data.attackTotalPower = totalPower;
			data.currentAttackElement = hand[bestIndex].GetType(); // ベースカードの属性

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

		// 攻撃のターゲットが人間以外（＝AIが防御する番）かチェック
		if (data.targetIdx != humanIdx && data.targetIdx >= 0 && data.targetIdx < (int)data.Player_Turn.size()) {

			Player& targetPlayer = data.Player_Turn[data.targetIdx];
			data.selectedDefenseCards.clear();

			int currentMp = targetPlayer.getMp();
			std::string incomingElement = data.currentAttackElement;

			// 属性で守れるカードを探してコンボにする
			for (int i = 0; i < (int)hand.size(); ++i) {
				CardCategory cat = hand[i].GetCategory();
				if ((cat == Defense || cat == Bilingual) && hand[i].GetMP() <= currentMp) {

					// 防御属性が合致する場合のみ追加
					if (DamageResolver::IsValidGuard(incomingElement, hand[i].GetType())) {
						data.selectedDefenseCards.push_back(i);
						currentMp -= hand[i].GetMP();
					}
				}
			}

			// 公開フェーズへ移行
			data.currentPhase = BattlePhase::DefenseReveal;
			data.revealIndex = 0;
			data.animationTimer = 15;
		}
	}
}