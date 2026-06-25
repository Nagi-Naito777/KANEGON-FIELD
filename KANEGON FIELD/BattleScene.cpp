#include "BattleScene.h"
#include "CardDatabase.h"
#include <algorithm>

// 戦闘ロジックの中身を見やすくするために分割したヘッダー
#include "BattleAIManager.h"
#include "BattleUIManager.h"
#include "BattleInputManager.h"
#include "BattleLogicManager.h"
#include "BattleData.h"

// コンストラクタの実装
BattleScene::BattleScene() {
	// 配列の初期化
	// ※fillを使えばアドレス指定で配列を初期化できる
	std::fill(std::begin(data.isHoverIdx), std::end(data.isHoverIdx), false);
	std::fill(std::begin(data.isHoverCardIdx), std::end(data.isHoverCardIdx), false);
	std::fill(std::begin(data.isHoverPlayerIdx), std::end(data.isHoverPlayerIdx), false);

	data.currentTurnIdx = 0;
}

// デストラクタの実装(実際はnewとか使った場合に使用する様子)
BattleScene::~BattleScene() {
}

void BattleScene::Initialize(const std::vector<Player>& initialPlayers) {
	// 完全初期化
	data.Clear();

	// 外部からちゃんとプレイヤーが渡された時だけ上書きする
	if (!initialPlayers.empty()) {
		data.Player_Turn = initialPlayers;
	}

	// 乱数エンジンのセットアップ
	std::random_device seed_gen;
	std::mt19937 engine(seed_gen());

	// AIの名前候補リスト
	std::vector<std::string> aiNames = {
		"ｼﾞﾝﾊﾞﾌﾞｴﾄﾞﾙ太郎", "ｷﾐﾉｶｾﾞﾊﾉﾄﾞｶﾗ", "カオスドゥラゴン", "破滅した世界", "アルコール", "ミセスタニシ",
		"木下 明憲", "アンドロイド伊藤", "消しゴムｽﾚｲﾔｰ", "マスターゴリラ", "ミーティング次郎", "ﾏｲｹﾙ･ｼﾞｪｲｸｿﾝ",
		"白川 真昼", "闇川 影虎", "森中 海導", "他人の鉛筆", "暴虐武人マン", "ナギナギ",
		"清水一登太郎", "怪盗マラカス", "イグラドガネ", "コハクンチョス", "リンクカネゴン", "ネットワーク",
		"System Error 404", "バチカン", "ブームブーム", "シャングリラ", "ｴﾝﾄﾞﾙｶﾈｺﾞﾝﾌｨｰﾙﾄﾞ", "アサアサ",
		"アラスカの風", "ハリwood", "マーKING飛高", "謝罪サムライ", "GPT", "EDM", ".cpp",
		"膝の上からｶﾝﾊﾟﾆｰ", "膝下ｽﾗｲﾃﾞｨﾝｸﾞ渉", "ワールドドリフ", ".h", "Destiny", "enum",
		"心の歪み", "憎悪", "深淵の戦士 ｱｽﾛﾝ", "leading", "string.h", "using",
		"黒魔術師 ﾅｲﾄﾒｱ", "神殺しのｱｻﾞﾘｵｽ", "闇の管理人", "エディション", "クラス.h", "カネゴンバレー",
		"砂岩ガン", "真夏の秋山", "真冬の春海", "ボンゴバナンザ", "ﾗｽﾄｵﾌﾞかねごん", "ﾘﾐﾃｯﾄﾞかねごん",
		"Kanegon", "ﾀﾞｰｸﾈｽｽﾏｲﾙ", "水しぶき", "かねごん動詞", "かねごん殴って", "終焉のかねごん",
		"雑草", "かん", "prism", "野菜", "厄災", "国王",
		"あまよもぎ", "ぁびゃ", "ユウキ", "中央都市かねごん", "かねごん禁忌", "かねごん構成",
		"カミヒデ", "カラムライア", "白川 大輔", "しずお", "1031", "Clover",
		"ナンバーコア", "キラ", "カンナ", "忠犬", "79わ", "ひんやり茶",
		"SML", "あ", "ああああああああ", "紫陽花", "ブーゲンビリア", "ﾀ",
		"ツチノコ", "ワシじゃよ、ワシ", "強すぎて滅", "マリオネット", "人生楽観思考", "雪谷 久代"
	};

	// AIの名前割り当て
	std::shuffle(aiNames.begin(), aiNames.end(), engine);

	int aiNameIdx = 0;
	for (auto& p : data.Player_Turn) {
		if (p.getControllerType() == ControllerType::AI) {
			std::string candidate = aiNames[aiNameIdx];
			aiNameIdx++;

			// g_player(ゲームプレイヤー)と名前が被らないようにするロジック
			if (candidate == g_player.getName() && aiNameIdx < (int)aiNames.size()) {
				candidate = aiNames[aiNameIdx];
				aiNameIdx++;
			}
			p.setName(candidate);
		}
	}

	// ターン順をシャッフル
	std::shuffle(data.Player_Turn.begin(), data.Player_Turn.end(), engine);
	// カード配布（ここで初期カードが配られるため、画面に描画されるようになります）
	for (auto& player : data.Player_Turn) {
		for (int i = 0; i < 9; ++i) {
			player.Hand.Add(CardDB.GetRandomCard());
		}
		player.Hand.Sort();
	}

	// 内部状態のリセット
	data.currentTurnIdx = 0;
	data.targetIdx = -1;
	data.playerTarget = false;
	data.selectedCards.clear();
	data.attackTotalPower = 0;
	data.defenseTotalPower = 0;
	data.currentPhase = BattlePhase::Select;
}

SceneName BattleScene::Update(const InputManager& input) {
	// プレイヤーがいなければ処理を止める
	if (data.Player_Turn.empty()) {
		return SceneName::BATTLE;
	}
	// =============================================================
	// 操作プレイヤー（人間）の情報を検索・特定する
	// =============================================================
	int humanIdx = 0;

	for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
		// 先ほど作った ControllerType::HUMAN で人間プレイヤーを探す
		if (data.Player_Turn[i].getControllerType() == ControllerType::HUMAN) {
			humanIdx = i;
			break;
		}
	}

	// 抽出した情報を変数にまとめる
	Player& humanPlayer = data.Player_Turn[humanIdx];
	bool isHumanTurn = (data.currentTurnIdx == humanIdx);

	// =============================================================
	// マネージャーの更新処理
	// =============================================================

	// 入力をデータに反映（先ほど作った引数をすべて渡す）
	bool isSurrender = inputManager.Update(data, input, humanPlayer, humanIdx, isHumanTurn);

	// もし降参（GIVE_UP）が選択されたら、戦闘シーンを終了する
	if (isSurrender) {
		return SceneName::SELECT;
	}

	// AIの思考をデータに反映
	aiManager.Update(data, humanIdx, isHumanTurn);

	// ルールに従ってデータを更新（ダメージ計算など）
	logicManager.Update(data);

	// =============================================================
	// シーンの継続
	// =============================================================
	return SceneName::BATTLE; // 通常時はそのまま戦闘シーンを続ける
}

void BattleScene::Draw() const {

	// 【重要】UIマネージャーが計算する前の「生のデータ」を画面に直接出す
	DrawFormatString(0, 0, Col.GetBla(), "Phase: %d", (int)data.currentPhase);
	DrawFormatString(0, 20, Col.GetBla(), "PlayerCount: %d", (int)data.Player_Turn.size());
	DrawFormatString(0, 40, Col.GetBla(), "TurnIdx: %d", data.currentTurnIdx);

    // 描画はデータを見てUIマネージャーに任せる
    uiManager.Draw(data);
}