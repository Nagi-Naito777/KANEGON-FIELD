#include "BattleScene.h"
#include "CardDatabase.h"
#include <algorithm>

// 戦闘ロジックの中身を見やすくするために分割したヘッダー
#include "BattleAIManager.h"
#include "BattleUIManager.h"
#include "BattleInputManager.h"
#include "BattleLogicManager.h"
#include "BattleData.h"

// 通信対戦用のネットワーク関係ヘッダー
#include "NetworkManager.h"

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
	// 操作プレイヤー（自分自身）の情報を特定する (共通処理)
	// =============================================================
	int myPlayerIdx = 0;
	for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
		// 人間（このPCを操作しているプレイヤー）を探す
		if (data.Player_Turn[i].getControllerType() == ControllerType::HUMAN) {
			myPlayerIdx = i;
			break;
		}
	}

	Player& myPlayer = data.Player_Turn[myPlayerIdx];
	bool isMyTurn = (data.currentTurnIdx == myPlayerIdx);

	// =============================================================
	// 通信状態の確認（オンラインかオフラインか）
	// =============================================================
	bool isOnline = (netManager != nullptr && netManager->IsConnected());

	if (isOnline) {
		// -------------------------------------------------------------
		// 【オンライン対戦の処理】(AI不使用・プレイヤー同士の対戦)
		// -------------------------------------------------------------

		// ① 【受信処理】相手やホストからのパケットを反映 (全員共通)
		GamePacket packet;
		while (netManager->PopPacket(packet)) {
			if (packet.type == CommandType::SYNC_PHASE) {
				data.currentPhase = static_cast<BattlePhase>(packet.value1);
				data.animFrame = 0;
				printfDx("同期: フェーズが %d に変わりました\n", packet.value1);
			}
			// 今後 ACTION_USE_CARD (カード選択) や END_TURN などの受信処理をここに追加
		}

		// ② 【入力・送信処理】自分の操作を処理
		// UI操作や自分のターンの入力受付を行う
		bool isSurrender = inputManager.Update(data, input, myPlayer, myPlayerIdx, isMyTurn);

		if (isSurrender) {
			// ※ 今後、降参したことを相手に伝えるパケットを送る処理が必要になります
			return SceneName::SELECT;
		}

		// ※ ここで inputManager で決定した行動（カード選択など）があれば、
		// クライアントなら SendPacket、ホストなら BroadcastPacket する処理を後々追加します。

		// ③ 【ロジック計算】ホストのみがゲーム進行を管理
		if (netManager->IsHost()) {
			// ダメージ計算やターンの進行など、ゲームの「正解」は審判であるホストだけが計算する
			logicManager.Update(data);

			// ※ ホスト側の logicManager でフェーズが変化したり、ダメージが発生した場合、
			// その結果を BroadcastPacket で全クライアントに送信し、同期させる処理を追加します。
		}
	}
	else {
		// -------------------------------------------------------------
		// 【AI対戦の処理】(オフライン時の従来のロジック)
		// -------------------------------------------------------------

		// ① プレイヤー（自分）の入力をデータに反映
		bool isSurrender = inputManager.Update(data, input, myPlayer, myPlayerIdx, isMyTurn);

		if (isSurrender) {
			return SceneName::SELECT;
		}

		// ② AIの思考をデータに反映 
		// ※ オンライン時はこのブロックに入らないため、対人戦でAIが勝手に動くことはありません。
		aiManager.Update(data, myPlayerIdx, isMyTurn);

		// ③ ルールに従ってデータを更新（ダメージ計算、フェーズ移行など）
		// オフラインなのでこのPC自身が全て計算する
		logicManager.Update(data);
	}

	// =============================================================
	// シーンの継続
	// =============================================================
	return SceneName::BATTLE;
}

void BattleScene::Draw() const {

	// 【重要】UIマネージャーが計算する前の「生のデータ」を画面に直接出す
	DrawFormatString(0, 0, Col.GetBla(), "Phase: %d", (int)data.currentPhase);
	DrawFormatString(0, 20, Col.GetBla(), "PlayerCount: %d", (int)data.Player_Turn.size());
	DrawFormatString(0, 40, Col.GetBla(), "TurnIdx: %d", data.currentTurnIdx);

    // 描画はデータを見てUIマネージャーに任せる
    uiManager.Draw(data);
}