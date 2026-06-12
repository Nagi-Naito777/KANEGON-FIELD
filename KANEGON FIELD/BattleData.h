#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include "GameConfig.h"
#include "Player.h"

enum class BattlePhase {
	Select,           // 攻撃側のカード選択
	AttackReveal,     // 攻撃カード公開演出
	TargetDisplay,    // ターゲット表示・確認
	DefenseSelect,    // 防御側のカード選択
	DefenseReveal,    // 防御カード公開演出
	Effect,           // ダメージ計算処理
	DamageResult,     // 結果表示・演出
	Idle			  // カード破棄・ドロー・次ターン準備
};

// 戦闘画面のボタン判定用の列挙体
enum BattleOption {
	NONE = -1,
	ATTACK,			// 攻撃決定判定枠
	DEFENSE,		// 防御決定判定枠
	RETURN,
	GIVE_UP,		// あきらめるボタンの判定枠
	MAX
};

struct BattleData {
	// --- 戦闘の進行・状態管理 ---
	BattlePhase currentPhase = BattlePhase::Select; // 現在のフェーズ
	std::vector<Player>Player_Turn;			// 参加プレイヤー一覧
	int currentTurnIdx;						// 現在のターンプレイヤーのインデックス
	bool isSurrenderConfirm;				// 降参確認ウィンドウの状態

	// --- 演出・アニメーション管理 ---
	int revealIndex = 0;					// カード公開演出用のインデックス
	int animationTimer = 0;					// アニメーション用のタイマー
	float currentYOffset = 65.0f;
	int animFrame = 0;						// フェーズ移行時に 0 にリセットする

	// --- カード選択・攻撃/防御の計算 ---
	int selectCard = -1;					// 現在選ばれてるカード
	std::vector<int> selectedCards;			// 選んだ手札のインデックスを順番に格納
	std::vector<int> selectedDefenseCards;	// 防御側が選択したカードのインデックス
	int attackTotalPower = 0;				// 重ね掛けした合計攻撃威力
	int defenseTotalPower = 0;				// 重ね掛けした合計防御威力
	std::string currentAttackElement = "無";// 現在の攻撃属性
	std::string currentDefenseElement = "無";// 現在の防御属性
	int targetIdx;							// マウスでホバーしたり選択した相手の番号
	bool playerTarget = false;				// プレイヤーを指定したかどうか

	// --- UI・マウス操作のフラグ ---
	int selectedOption;						// 現在選ばれている選択肢
	int selectPlayer;						// 現在選ばれてるプレイヤー
	int hoveredCardIdx;						// マウスカーソルで選択しているカード番号

	// --- ホバー判定配列（マウス入力管理用） ---
	bool isHoverIdx[MAX];					// 各ボタンの上にマウスがあるか
	bool isHoverCardIdx[CARD_MAX];			// カード枠の上にマウスがあるか
	bool isHoverPlayerIdx[MEMBER_MAX];		// どのプレイヤー枠の上にマウスがあるか

	// データを初期状態に戻すための関数
	void Clear() {
		// --- 配列のリセット ---
		std::fill(std::begin(isHoverIdx), std::end(isHoverIdx), false);
		std::fill(std::begin(isHoverCardIdx), std::end(isHoverCardIdx), false);
		std::fill(std::begin(isHoverPlayerIdx), std::end(isHoverPlayerIdx), false);

		// --- 進行・状態管理のリセット ---
		currentPhase = BattlePhase::Select;
		currentTurnIdx = 0;
		isSurrenderConfirm = false;

		// --- 演出・タイマーのリセット ---
		revealIndex = 0;
		animationTimer = 0;

		// --- カード・数値関連のリセット ---
		selectCard = -1;
		selectedCards.clear();
		selectedDefenseCards.clear();
		attackTotalPower = 0;
		defenseTotalPower = 0;
		currentAttackElement = "無";
		currentDefenseElement = "無";
		targetIdx = -1;
		playerTarget = false;

		// --- UI・操作フラグのリセット ---
		selectedOption = BattleOption::NONE;
		selectPlayer = -1;
		hoveredCardIdx = -1;

		// ※ Player_Turn は Initialize で再代入されるため、
		// ここで clear しても問題ありません。
		Player_Turn.clear();
	}
};