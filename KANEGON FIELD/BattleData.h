#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include "GameConfig.h"
#include "Player.h"

// ログのタイプ判別用
enum class PopupType {
	Number,		// 通常のダメージ数値や回復数値を出す
	Text		// 弾きやカウンター系のフォントを出す
};

// ログの全体の種類
enum class LogType {
	Damage,		// ダメージログ
	Heal,		// HP回復ログ
	MagicHeal,	// MP回復ログ
	Money,		// お金関係ログ
	Hit,		// 全体攻撃に当たったかどうかログ
	YamiDama,	// 闇属性の追撃ダメージログ
	Counter,	// 跳ね返しログ
	Parry,		// 弾きログ
	NoHit		// 無効化ログ
};

// バトルログ1行分のデータ
struct BattleLog {
	std::string text;
	LogType type;
	unsigned int color;
};

enum class BattlePhase {
	Select,           // 攻撃側のカード選択
	ChangeStatusEdit, // 換カードのステータス調整待ち
	BuyConfirm,       // 買カードの購入確認待ち
	AttackReveal,     // 攻撃カード公開演出
	TargetDisplay,    // ターゲット表示・確認
	DefenseSelect,    // 防御側のカード選択
	DefenseReveal,    // 防御カード公開演出
	Effect,           // ダメージ計算処理
	DamageResult,     // 結果表示・演出
	End,              // 戦闘終了リザルト
	Idle              // カード破棄・ドロー・次ターン準備
};

// 戦闘画面のボタン判定用の列挙体
enum BattleOption {
	NONE = -1,
	ATTACK,			// 攻撃決定判定枠
	DEFENSE,		// 防御決定判定枠
	RETURN,
	GIVE_UP,		// あきらめるボタンの判定枠
	BUY_YES,		// 買カードの「買う」ボタン
	BUY_NO,			// 買カードの「買わない」ボタン
	MP_ADD,			// 換カードのMP増加ボタン
	MP_DOWN,		// 換カードのMP減少ボタン
	MONEY_ADD,		// 換カードの金額増加ボタン
	MONEY_DOWN,		// 換カードの金額減少ボタン
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
	std::string currentDefenseElement = "無";//現在の防御属性
	int targetIdx;							// マウスでホバーしたり選択した相手の番号
	bool playerTarget = false;				// プレイヤーを指定したかどうか
	int extraAttackPower = 0;				// 「アンリミテッド」などの固定追加ダメージ用

	// --- 換・買カード用の一時保存用変数 ---
	int changeMP = 0;						// 換カードで変動させるMP量
	int changeMoney = 0;					// 換カードで変動させる金額量
	int buyTargetCardIdx = -1;				// 買カードで購入しようとしているカードのインデックス
	int sellTargetCardIdx = -1;				// 売カードで売りつけるために選んだ手札インデックス（selectedCardsの2枚目）

	// --- 特殊効果フラグ（ターン中に効果が持続するもの） ---
	bool isAllAttack = false;				// 全体攻撃化フラグ
	float attackMultiplier = 1.0f;			// 攻撃倍率変更変数
	bool isImmune = false;					// 攻撃無効化フラグ
	bool isParry = false;					// 攻撃を弾くフラグ
	bool isCounter = false;					// 攻撃を跳ね返す(カウンター)フラグ
	bool isDrain = false;					// HP吸収フラグ

	// --- カウンター（攻守逆転・連鎖）用の保持変数 ---
	int originalTurnIdx = -1;               // カウンター発生前の本来のターンプレイヤーインデックス
	bool isPendingAttack = false;           // カウンターなどの「保留中の攻撃」があるか
	int pendingAttackPower = 0;             // 保留中のカウンター攻撃力
	std::string pendingAttackType = "無";   // 保留中のカウンター属性

	// --- UI・マウス操作のフラグ ---
	int selectedOption;						// 現在選ばれている選択肢
	int selectPlayer;						// 現在選ばれてるプレイヤー
	int hoveredCardIdx;						// マウスカーソルで選択しているカード番号

	// --- ホバー判定配列（マウス入力管理用） ---
	bool isHoverIdx[MAX];					// 各ボタンの上にマウスがあるか
	bool isHoverCardIdx[CARD_MAX];			// カード枠の上にマウスがあるか
	bool isHoverPlayerIdx[MEMBER_MAX];		// どのプレイヤー枠の上にマウスがあるか

	// --- UI表示用の確定リザルトデータ ---
	int lastDamageDealt = 0;                // 直前に与えた確定ダメージ量
	int lastHealingDone = 0;                // 直前に回復した確定回復量
	int resultTargetIdx = -1;               // ダメージや回復の影響を受けたプレイヤーのインデックス

	// アニメーション表示用の枚数
	int animAttackCardCount = 0;  // 攻撃側が出すカードの表示枚数
	int animDefenseCardCount = 0; // 防御側が出すカードの表示枚数

	// 選択ロックの配列（手札の数だけ用意し、ロジック側でtrue/falseを判定しておく）
	std::vector<bool> isCardSelectable;

	// ダメージや回復数値を表示するポップアップUI用の構造体
	struct EffectPopup {
		PopupType type;      // 演出の種類
		int playerIdx;       // 対象プレイヤー（キャラの頭上に出すため）
		std::string text;    // 表示テキスト（数値も文字列化してここに入れる）
		unsigned int color;  // 色
		int offsetY;         // Y座標オフセット（アニメーション用）
		int timer;           // 現在の残り表示時間
		int maxTimer;        // 初期の表示時間（フェードアウトの透明度計算用）

		// 初期化用コンストラクタ (timer を maxTimer にも自動セット)
		EffectPopup(PopupType t, int pIdx, std::string txt, unsigned int c, int offY, int time)
			: type(t), playerIdx(pIdx), text(txt), color(c), offsetY(offY), timer(time), maxTimer(time) {}
	};
	std::vector<EffectPopup> popups; // 発生中のポップアップリスト

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

		// --- 換・買カード用の一時保存用変数 ---
		changeMP = 0;
		changeMoney = 0;
		buyTargetCardIdx = -1;

		// --- 特殊効果フラグ ---
		isAllAttack = false;
		attackMultiplier = 1.0f;
		isImmune = false;
		isParry = false;
		isCounter = false;
		isDrain = false;

		// --- カウンター用のリセット ---
		originalTurnIdx = -1;
		isPendingAttack = false;
		pendingAttackPower = 0;
		pendingAttackType = "無";

		// --- UI・操作フラグのリセット ---
		selectedOption = BattleOption::NONE;
		selectPlayer = -1;
		hoveredCardIdx = -1;

		// --- UIリザルト用のリセット ---
		lastDamageDealt = 0;
		lastHealingDone = 0;
		resultTargetIdx = -1;

		// --- ポップアップ消去 ---
		popups.clear();

		// ※ Player_Turn は Initialize で再代入されるため、
		// ここで clear しても問題ありません。
		Player_Turn.clear();
	}
};