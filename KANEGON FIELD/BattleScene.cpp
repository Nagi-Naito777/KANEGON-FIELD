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
	// メンバ変数の初期化
	data.Clear();
	localData.Clear();
}

// デストラクタの実装(実際はnewとか使った場合に使用する様子)
BattleScene::~BattleScene() {
}

void BattleScene::Initialize(const std::vector<Player>& initialPlayers) {
	// 完全初期化
	data.Clear();
	localData.Clear();

	// 外部からちゃんとプレイヤーが渡された時だけ上書きする
	if (!initialPlayers.empty()) {
		data.Player_Turn = initialPlayers;
	}

	// オンラインかどうかを判定
	bool isOnline = (netManager != nullptr && netManager->IsConnected());
	printfDx("DEBUG: netManager pointer: %p, isConnected: %d\n", (void*)netManager, (int)(netManager ? netManager->IsConnected() : -1));

	// --- 【重要】AI対戦（オフライン）の時だけ実行する処理 ---
	if (!isOnline) {
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

	}

	// カード配布
	for (auto& player : data.Player_Turn) {
		for (int i = 0; i < 9; ++i) {
			player.Hand.Add(CardDB.GetRandomCard());
		}
		player.Hand.Sort();
	}

	// --- 自分のプレイヤーIDを最初に特定して保持する ---
	localData.myPlayerIndex = 0;
	for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
		if (data.Player_Turn[i].getControllerType() == ControllerType::HUMAN) {
			localData.myPlayerIndex = i;
			break;
		}
	}

	data.currentTurnIdx = 0;
	data.currentPhase = BattlePhase::Select;
}

SceneName BattleScene::Update(const InputManager& input) {
	if (data.Player_Turn.empty()) {
		return SceneName::BATTLE;
	}

	Player& myPlayer = data.Player_Turn[localData.myPlayerIndex];
	bool isMyTurn = (data.currentTurnIdx == localData.myPlayerIndex);

	// 入力更新
	PlayerAction myAction = inputManager.Update(data, localData, input, myPlayer, localData.myPlayerIndex, isMyTurn);
	if (myAction.isSurrender) return SceneName::SELECT;

	bool isOnline = (netManager != nullptr && netManager->IsConnected());

	// =============================================================
	// 通信対戦時の処理
	// =============================================================
	if (isOnline) {
		// 1. パケット受信処理（ホスト・クライアント共通）
		GamePacket packet;
		while (netManager->PopPacket(packet)) {
			printfDx("DEBUG: Received Packet Type: %d\n", (int)packet.type);

			// 【共通】フェーズの同期
			if (packet.type == CommandType::SYNC_PHASE) {
				data.currentPhase = static_cast<BattlePhase>(packet.value1);
				localData.animFrame = 0;
			}

			// 【ホスト側】クライアントからのアクションを受信
			if (netManager->IsHost()) {
				if (packet.type == CommandType::CLIENT_ACTION) {
					// クライアントのアクション（選んだカードや対象）をホストの data に反映する
					// 例: 
					// data.Player_Turn[packet.clientID].selectedCard = packet.value1;
					// data.targetIdx = packet.value2;

					// アクションを受け取ったのでフェーズを進行させる
					BattlePhase nextPhase = BattlePhase::DefenseSelect;
					data.currentPhase = nextPhase;
					localData.animFrame = 0;

					GamePacket syncP;
					syncP.type = CommandType::SYNC_PHASE;
					syncP.value1 = (int)nextPhase;
					netManager->BroadcastPacket(syncP);
				}
			}
			// 【クライアント側】ホストからのゲームデータ同期を受信
			else {
				if (packet.type == CommandType::SYNC_GAME_DATA) {
					// ホストから送られてきたHP、手札情報、ターンの状態などで自身の data を上書きする
					// 例:
					// data.currentTurnIdx = packet.value1;
					// data.Player_Turn[packet.targetPlayer].HP = packet.value2;
					// ※GamePacketの構造に合わせて必要なデータを反映してください
				}
			}
		}

		// 2. アクション送信処理
		if (myAction.hasAction) {
			if (myAction.isAttackDecision) {
				if (netManager->IsHost()) {
					// ホストは直接フェーズ更新
					BattlePhase nextPhase = myAction.isHealAction ? BattlePhase::DefenseReveal : BattlePhase::DefenseSelect;
					data.currentPhase = nextPhase;
					localData.animFrame = 0;

					GamePacket syncPacket = { CommandType::SYNC_PHASE, (int)data.currentPhase };
					netManager->BroadcastPacket(syncPacket);
				}
				else {
					// クライアントは自分のアクション内容（選択カード等）をホストに送信する
					printfDx("DEBUG: Sending CLIENT_ACTION request\n");
					GamePacket actionPacket;
					actionPacket.type = CommandType::CLIENT_ACTION;
					// actionPacket.value1 = myAction.selectedCardID;     // 例
					// actionPacket.value2 = myAction.targetPlayerIndex;  // 例
					netManager->SendPacket(actionPacket);
				}
			}
		}

		// 3. ゲーム進行権限（ホストのみがロジックを回す）
		if (netManager->IsHost()) {
			// ホスト側で戦闘ロジックの計算とAIの処理を行う
			logicManager.Update(data, localData);
			aiManager.Update(data);

			// ※ data.isChanged フラグ等をBattleData.hに定義しておき、
			// ターン経過やダメージ処理が起きた時に true にする設計が推奨です。
			// ここでは毎フレーム送らないための仮のフラグ制御を想定しています。
			bool needsSync = true; // 実際は data.isChanged などを使用してください

			if (needsSync) {
				// プレイヤー全員分のデータを順番に送信する
				for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
					GamePacket syncDataPacket;
					syncDataPacket.type = CommandType::SYNC_GAME_DATA;

					syncDataPacket.value1 = i; // どのプレイヤーのデータか
					syncDataPacket.value2 = data.Player_Turn[i].getHp(); // 現在のHP
					syncDataPacket.value3 = data.currentTurnIdx; // 現在のターンプレイヤー

					// 手札の同期 (最大10枚想定)
					// ※Handクラスの仕様に合わせてIDを取得してください
					for (int c = 0; c < MAX_HAND_CARD; ++c) {
						if (c < data.Player_Turn[i].Hand.GetCount()) {
							syncDataPacket.cardIds[c] = data.Player_Turn[i].Hand.GetCards();
						}
						else {
							syncDataPacket.cardIds[c] = -1; // 空枠
						}
					}

					netManager->BroadcastPacket(syncDataPacket);
				}
				// data.isChanged = false; // 送信が終わったらフラグを戻す
			}
		}
	}
	// =============================================================
	// AI対戦（オフライン）の処理
	// =============================================================
	else {
		if (myAction.hasAction) {
			if (myAction.isAttackDecision) {
				data.currentPhase = myAction.isHealAction ? BattlePhase::DefenseReveal : BattlePhase::DefenseSelect;
				localData.animFrame = 0;
			}
			else if (myAction.isDefenseDecision) {
				data.currentPhase = BattlePhase::DefenseReveal;
				localData.animFrame = 0;
			}
		}

		// オフライン時は自分たちで進行させる
		aiManager.Update(data);
		logicManager.Update(data, localData);
	}

	return SceneName::BATTLE;
}

void BattleScene::Draw() const {
	// デバッグ用表示などもlocalDataの情報を考慮可能
	DrawFormatString(0, 0, Col.GetBla(), "Phase: %d", (int)data.currentPhase);
	DrawFormatString(0, 20, Col.GetBla(), "MyIndex: %d", localData.myPlayerIndex);

	// 【重要】UIマネージャーには両方のデータを渡す
	// data: 全員共通のゲーム状態
	// localData: 自分の端末のUI状態
	uiManager.Draw(data, localData);
}