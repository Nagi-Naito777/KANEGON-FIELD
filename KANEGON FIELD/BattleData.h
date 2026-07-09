#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include "GameConfig.h"
#include "Player.h"

// ログのタイプ判別用
enum class PopupType {
	Damage,		// ダメージ
	Heal,		// HP回復
	MagicHeal,	// MP回復
	Money,		// お金関係
	Hit,		// 全体攻撃に当たったかどうか
	YamiDama,	// 闇属性の追撃ダメージ
	Counter,	// 跳ね返し
	Parry,		// 弾き
	NoHit		// 無効化
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

// ゲームモードの定義（乱闘 or タイマン）
enum class GameMode {
	BRAWL,			// 乱闘モード（最大9人）
	SERIOUS_DUEL	// 真剣勝負モード（1対1の2人対戦）
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

// ============================================================================
// 【共有データ】通信で全員と同期する絶対的なゲームの状態（サーバー・ホスト基準）
// ============================================================================
struct BattleData {
	// --- 戦闘の進行・状態管理 ---
	GameMode currentMode = GameMode::BRAWL;	// 現在のゲームモード
	BattlePhase currentPhase = BattlePhase::Select; // 現在のフェーズ
	std::vector<Player> Player_Turn;		// 参加プレイヤー一覧
	int currentTurnIdx;						// 現在のターンプレイヤーのインデックス

	// 通信同期用の制御フラグ
	bool isChanged = false;

	// --- 確定した行動データ（全員の画面に反映されるもの） ---
	std::vector<int> confirmedAttackCards;	// 決定ボタンを押して確定した攻撃カード
	std::vector<int> confirmedDefenseCards; // 決定ボタンを押して確定した防御カード
	int targetIdx;							// 確定したターゲットの番号
	bool playerTarget = false;				// プレイヤーを指定したかどうか

	// --- 攻撃/防御の計算結果 ---
	int attackTotalPower = 0;				// 重ね掛けした合計攻撃威力
	int defenseTotalPower = 0;				// 重ね掛けした合計防御威力
	std::string currentAttackElement = "無";// 現在の攻撃属性
	std::string currentDefenseElement = "無";//現在の防御属性
	int extraAttackPower = 0;				// 「アンリミテッド」などの固定追加ダメージ用

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

	// --- 全体通知用の確定リザルトデータ ---
	int lastDamageDealt = 0;                // 直前に与えた確定ダメージ量
	int lastHealingDone = 0;                // 直前に回復した確定回復量
	int resultTargetIdx = -1;               // ダメージや回復の影響を受けたプレイヤーのインデックス

	void Clear() {
		currentPhase = BattlePhase::Select;
		currentTurnIdx = 0;
		Player_Turn.clear();

		isChanged = false;

		confirmedAttackCards.clear();
		confirmedDefenseCards.clear();
		targetIdx = -1;
		playerTarget = false;

		attackTotalPower = 0;
		defenseTotalPower = 0;
		currentAttackElement = "無";
		currentDefenseElement = "無";
		extraAttackPower = 0;

		isAllAttack = false;
		attackMultiplier = 1.0f;
		isImmune = false;
		isParry = false;
		isCounter = false;
		isDrain = false;

		originalTurnIdx = -1;
		isPendingAttack = false;
		pendingAttackPower = 0;
		pendingAttackType = "無";

		lastDamageDealt = 0;
		lastHealingDone = 0;
		resultTargetIdx = -1;
	}
};

// ============================================================================
// 【ローカルデータ】自分の画面だけに存在する演出・UI・入力途中のデータ
// ============================================================================

// ダメージや回復数値を表示するポップアップUI用の構造体（ローカル描画用）
struct EffectPopup {
	PopupType type;      // 演出の種類
	int playerIdx;       // 対象プレイヤー
	std::string text;    // 表示テキスト
	unsigned int color;  // 色
	int offsetY;         // Y座標オフセット
	int timer;           // 現在の残り表示時間
	int maxTimer;        // 初期の表示時間

	EffectPopup(PopupType t, int pIdx, std::string txt, unsigned int c, int offY, int time)
		: type(t), playerIdx(pIdx), text(txt), color(c), offsetY(offY), timer(time), maxTimer(time) {}
};

struct LocalClientData {
	int myPlayerIndex = -1;					// 自分がPlayer_Turnの何番目か

	// --- ローカルでの選択状態（決定ボタンを押す前） ---
	std::vector<int> localSelectingCards;	// 手札から選んでプレビューしているカード
	int localTargetIdx = -1;				// マウスでクリックして仮決めしているターゲット

	// --- UI・マウス操作のフラグ ---
	int selectedOption = BattleOption::NONE;
	int selectPlayer = -1;
	int hoveredCardIdx = -1;
	bool isSurrenderConfirm = false;		// 降参確認ウィンドウの表示状態

	// --- ホバー判定配列（マウス入力管理用） ---
	bool isHoverIdx[MAX];
	bool isHoverCardIdx[CARD_MAX];
	bool isHoverPlayerIdx[MEMBER_MAX];

	// --- 換・買カード用の一時保存用変数（ローカル操作用） ---
	int changeMP = 0;
	int changeMoney = 0;
	bool isBuyingAction = false;
	int buyTargetCardIdx = -1;
	bool isSellingAction = false;
	int sellTargetCardIdx = -1;

	// --- 演出・アニメーション管理 ---
	int revealIndex = 0;
	int animationTimer = 0;
	float currentYOffset = 65.0f;
	int animFrame = 0;
	int animAttackCardCount = 0;
	int animDefenseCardCount = 0;

	// --- UI状態管理 ---
	std::vector<bool> isCardSelectable;		// 選択ロックの配列
	std::vector<EffectPopup> popups;		// 発生中のポップアップリスト

	void Clear() {
		localSelectingCards.clear();
		localTargetIdx = -1;

		selectedOption = BattleOption::NONE;
		selectPlayer = -1;
		hoveredCardIdx = -1;
		isSurrenderConfirm = false;

		std::fill(std::begin(isHoverIdx), std::end(isHoverIdx), false);
		std::fill(std::begin(isHoverCardIdx), std::end(isHoverCardIdx), false);
		std::fill(std::begin(isHoverPlayerIdx), std::end(isHoverPlayerIdx), false);

		changeMP = 0;
		changeMoney = 0;
		isBuyingAction = false;
		buyTargetCardIdx = -1;
		isSellingAction = false;
		sellTargetCardIdx = -1;

		revealIndex = 0;
		animationTimer = 0;
		currentYOffset = 65.0f;
		animFrame = 0;
		animAttackCardCount = 0;
		animDefenseCardCount = 0;

		isCardSelectable.clear();
		popups.clear();
	}
};