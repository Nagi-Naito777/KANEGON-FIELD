#include "BattleAIManager.h"
#include "BattleData.h"
#include "Card.h"
#include <vector>
#include <random>

void BattleAIManager::Update(BattleData& data, int humanIdx, bool isHumanTurn) {
	// 人間のターン時のSelectフェーズならAIは行動しない
	if (isHumanTurn && data.currentPhase == BattlePhase::Select) return;

	Player& turnPlayer = data.Player_Turn[data.currentTurnIdx];

	// =============================================================
	// 攻撃フェーズ（AIのターン時）
	// =============================================================
	if (data.currentPhase == BattlePhase::Select && !isHumanTurn) {

		data.selectedCards.clear();

		// 手札ベクターを PlayerHand クラスから取得
		const auto& hand = turnPlayer.Hand.GetCards();

		int bestIndex = -1;
		int maxPower = -1;

		// --- 手札から【MPが足りる】かつ【一番強い】武器や魔法を選ぶ ---
		for (int i = 0; i < (int)hand.size(); ++i) {
			CardCategory cat = hand[i].GetCategory();
			// ※ CardCategory::Attack などの場合は適宜プレフィックスを付けてください
			if (cat == Attack || cat == Magic || cat == Bilingual) {
				// MPが足りているかチェック
				if (turnPlayer.getMp() >= hand[i].GetMP()) {
					// より攻撃力が高いカードを記憶する
					if (hand[i].GetPower() > maxPower) {
						maxPower = hand[i].GetPower();
						bestIndex = i;
					}
				}
			}
		}

		// --- 攻撃カードが選べた場合 ---
		if (bestIndex != -1) {
			data.selectedCards.push_back(bestIndex);

			// 選んだカードの威力と属性をデータにセット
			data.totalPower = hand[bestIndex].GetPower();
			std::string type = hand[bestIndex].GetType();
			data.currentAttackElement = (type == "") ? "無" : type;

			// 生きている敵（自分以外）からランダムにターゲットを選ぶ
			std::vector<int> aliveEnemies;
			for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
				// ★修正: Status.dead を参照して気絶判定
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
			// --- 攻撃できるカードがない場合（パス） ---
			data.playerTarget = false;
			data.targetIdx = -1;
			data.totalPower = 0;
			data.currentAttackElement = "無";

			// （ターンインデックスを次に進めて、Selectフェーズのままにする）
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

			// ★修正: 手札ベクターを PlayerHand クラスから取得
			const auto& hand = targetPlayer.Hand.GetCards();
			int bestIndex = -1;
			int maxPower = -1;

			// --- 防御カードを検索（MPが足りる中で一番防御力が高いもの） ---
			for (int i = 0; i < (int)hand.size(); ++i) {
				CardCategory cat = hand[i].GetCategory();
				if (cat == Defense || cat == Bilingual) {
					// MPチェック
					if (targetPlayer.getMp() >= hand[i].GetMP()) {
						if (hand[i].GetPower() > maxPower) {
							maxPower = hand[i].GetPower();
							bestIndex = i;
						}
					}
				}
			}

			// 防御カードが見つかったらセット
			if (bestIndex != -1) {
				data.selectedDefenseCards.push_back(bestIndex);
			}

			// 防御できても、カードがなくて防御できなくても Reveal（公開）フェーズへ
			data.currentPhase = BattlePhase::Reveal;
			data.revealIndex = 0;
			data.animationTimer = 15;
		}
	}
}