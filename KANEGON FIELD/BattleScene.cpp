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

	// --- AI対戦（オフライン）の時だけ実行する処理 ---
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

	// カード初期配布（ホストまたはオフラインのみ）
	if (!isOnline || (isOnline && netManager->IsHost())) {
		for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
			data.Player_Turn[i].Hand.Clear();
			for (int c = 0; c < START_CARD; ++c) {
				Card drawnCard = CardDB.GetRandomCard();
				data.Player_Turn[i].Hand.Add(drawnCard);
			}
			data.Player_Turn[i].Hand.Sort();
		}
	}

	// ==============================================================
	// ★【重要修正】通信によるID割り当てを廃止し、直接IDを決定する
	// ==============================================================
	if (isOnline) {
		if (netManager->IsHost()) {
			localData.myPlayerIndex = 0; // ホストは確定で0
			printfDx("DEBUG: [INIT] I am Host. Assigned ID: 0\n");
		}
		else {
			localData.myPlayerIndex = 1; // 2人対戦前提：クライアントは確定で1
			printfDx("DEBUG: [INIT] I am Client. Assigned ID: 1\n");
		}
	}
	else {
		// オフライン用
		localData.myPlayerIndex = 0;
		for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
			if (data.Player_Turn[i].getControllerType() == ControllerType::HUMAN) {
				localData.myPlayerIndex = i;
				break;
			}
		}
	}

	data.currentTurnIdx = 0;
	data.currentPhase = BattlePhase::Select;

	if (isOnline && netManager->IsHost()) {
		data.isChanged = true;
	}
}

SceneName BattleScene::Update(const InputManager& input) {
	bool isOnline = (netManager != nullptr && netManager->IsConnected());

	if (isOnline) {
		// =========================================================================
		// ★【最重要修正】毎フレームネットワークの受信キューを更新する！
		// これがないと、裏側で届いたパケットが packetQueue に入らず受信処理が一生回りません
		// =========================================================================
		netManager->Update();

		GamePacket packet;
		while (netManager->PopPacket(packet)) {
			// 【全員共通】フェーズの強制同期
			if (packet.type == (int)CommandType::SYNC_PHASE) {
				data.currentPhase = static_cast<BattlePhase>(packet.value1);
				localData.animFrame = 0;
			}

			// 【ホスト側】クライアント（相手）からのカード決定アクションを受信
			if (netManager->IsHost()) {
				if (packet.type == (int)CommandType::CLIENT_ACTION) {
					int clientId = packet.value3;
					data.targetIdx = packet.value2;

					// クライアントが「攻撃」を決定した時
					if (data.currentPhase == BattlePhase::Select) {
						data.confirmedAttackCards.clear();
						for (int c = 0; c < MAX_HAND_CARD; ++c) {
							if (packet.cardIds[c] != -1) {
								data.confirmedAttackCards.push_back(packet.cardIds[c]);
							}
						}
						data.currentPhase = BattlePhase::DefenseSelect; // 防御側選択フェーズへ移行
						localData.animFrame = 0;
						data.isChanged = true;
					}
					// クライアントが「防御」を決定した時
					else if (data.currentPhase == BattlePhase::DefenseSelect) {
						data.confirmedDefenseCards.clear();
						for (int c = 0; c < MAX_HAND_CARD; ++c) {
							if (packet.cardIds[c] != -1) {
								data.confirmedDefenseCards.push_back(packet.cardIds[c]);
							}
						}
						data.currentPhase = BattlePhase::DefenseReveal; // 結果発表フェーズへ移行
						localData.animFrame = 0;
						data.isChanged = true;
					}
				}
			}
			// 【クライアント側】ホストからの最新ゲーム状態の同期を受信
			else {
				if (packet.type == (int)CommandType::SYNC_GAME_DATA) {
					int targetIdx = packet.value1;

					while ((int)data.Player_Turn.size() <= targetIdx) {
						data.Player_Turn.emplace_back();
					}

					data.Player_Turn[targetIdx].setHp(packet.value2);
					data.currentTurnIdx = packet.value3;
					data.currentPhase = static_cast<BattlePhase>(packet.value4);
					data.targetIdx = packet.value5;

					// ★【新方式】フェーズ連動型同期（インデックスのズレ、バグを100%防止）
					// 攻撃フェーズまたは防御選択フェーズなら、送られてきたのは「攻撃カード」
					if (data.currentPhase == BattlePhase::Select || data.currentPhase == BattlePhase::DefenseSelect) {
						data.confirmedAttackCards.clear();
						for (int c = 0; c < MAX_HAND_CARD; ++c) {
							int cardId = packet.cardIds[c];
							if (cardId != -1) data.confirmedAttackCards.push_back(cardId);
						}
						data.confirmedDefenseCards.clear(); // まだ防御前なのでクリア
					}
					// それ以降のフェーズなら、送られてきたのは「防御カード」
					else {
						data.confirmedDefenseCards.clear();
						for (int c = 0; c < MAX_HAND_CARD; ++c) {
							int cardId = packet.cardIds[c];
							if (cardId != -1) data.confirmedDefenseCards.push_back(cardId);
						}
					}
				}
				else if (packet.type == (int)CommandType::SYNC_PRIVATE_HAND) {
					int targetIdx = packet.value1;

					// ★【重要修正】自分だけでなく「相手の手札」も強制的に同期するように変更！
					// これにより、UI描画マネージャー（uiManager）が相手の手札（裏向き画像や枚数）を画面に正しく描画できるようになります。
					if (targetIdx >= 0) {
						while ((int)data.Player_Turn.size() <= targetIdx) {
							data.Player_Turn.emplace_back();
						}

						data.Player_Turn[targetIdx].Hand.Clear();
						for (int c = 0; c < MAX_HAND_CARD; ++c) {
							int cardId = packet.cardIds[c];
							if (cardId >= 0 && cardId < CARD_KIND) {
								data.Player_Turn[targetIdx].Hand.Add(CardDB.GetCardByID(cardId));
							}
						}
						data.Player_Turn[targetIdx].Hand.Sort();
					}
				}
			}
		}
	}

	// プレイヤーデータ準備チェック（ガード節）
	if (data.Player_Turn.empty() || localData.myPlayerIndex < 0 || localData.myPlayerIndex >= (int)data.Player_Turn.size()) {
		return SceneName::BATTLE;
	}

	// ローカルプレイヤーの入力処理
	Player& myPlayer = data.Player_Turn[localData.myPlayerIndex];
	bool isMyTurn = (data.currentTurnIdx == localData.myPlayerIndex);

	PlayerAction myAction = inputManager.Update(data, localData, input, myPlayer, localData.myPlayerIndex, isMyTurn);
	if (myAction.isSurrender) return SceneName::SELECT;

	// アクション送信 / ホストのゲーム進行処理
	if (isOnline) {
		if (myAction.hasAction) {
			// 【ホスト自身が操作して決定した場合】
			if (netManager->IsHost()) {
				if (myAction.isAttackDecision) {
					// ホストが選んだ攻撃カードを確定枠に直接反映
					data.confirmedAttackCards.clear();
					for (int cardId : myAction.selectedCardIdxs) {
						data.confirmedAttackCards.push_back(cardId);
					}
					data.currentPhase = myAction.isHealAction ? BattlePhase::DefenseReveal : BattlePhase::DefenseSelect;
					localData.animFrame = 0;
					data.isChanged = true;
				}
				else if (myAction.isDefenseDecision) {
					// ホストが選んだ防御カードを確定枠に直接反映
					data.confirmedDefenseCards.clear();
					for (int cardId : myAction.selectedCardIdxs) {
						data.confirmedDefenseCards.push_back(cardId);
					}
					data.currentPhase = BattlePhase::DefenseReveal;
					localData.animFrame = 0;
					data.isChanged = true;
				}
			}
			// 【クライアントが操作して決定した場合】ホストへパケットを送信
			else {
				if (myAction.isAttackDecision || myAction.isDefenseDecision) {
					GamePacket actionPacket;
					memset(&actionPacket, 0, sizeof(GamePacket));
					actionPacket.type = (int)CommandType::CLIENT_ACTION;
					actionPacket.value2 = myAction.targetIdx;
					actionPacket.value3 = localData.myPlayerIndex;

					for (int c = 0; c < MAX_HAND_CARD; ++c) {
						if (c < (int)myAction.selectedCardIdxs.size()) {
							actionPacket.cardIds[c] = myAction.selectedCardIdxs[c];
						}
						else {
							actionPacket.cardIds[c] = -1;
						}
					}
					netManager->SendPacket(actionPacket);
				}
			}
		}

		// 【ホスト側限定】ゲーム状態の自動更新と、全クライアントへのデータ定期送信
		if (netManager->IsHost()) {
			BattlePhase oldPhase = data.currentPhase;
			int oldTurnIdx = data.currentTurnIdx;
			int oldTargetIdx = data.targetIdx;
			std::vector<int> oldHps(data.Player_Turn.size());
			for (size_t i = 0; i < data.Player_Turn.size(); ++i) {
				oldHps[i] = data.Player_Turn[i].getHp();
			}

			logicManager.Update(data, localData);
			aiManager.Update(data);

			// 状態の変化を検知
			if (data.currentPhase != oldPhase || data.currentTurnIdx != oldTurnIdx || data.targetIdx != oldTargetIdx) {
				data.isChanged = true;
			}
			for (size_t i = 0; i < data.Player_Turn.size(); ++i) {
				if (data.Player_Turn[i].getHp() != oldHps[i]) {
					data.isChanged = true;
				}
			}

			// 定期送信タイマー（30フレームに1回強制同期）
			static int autoSyncTimer = 0;
			autoSyncTimer++;
			if (autoSyncTimer >= 30) {
				data.isChanged = true;
				autoSyncTimer = 0;
			}

			// データに変更があった場合、全員へブロードキャスト
			if (data.isChanged) {
				// 1. 基本ゲームデータと「場に出たカード」の送信
				for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
					GamePacket syncDataPacket;
					memset(&syncDataPacket, 0, sizeof(GamePacket));

					syncDataPacket.type = (int)CommandType::SYNC_GAME_DATA;
					syncDataPacket.value1 = i;
					syncDataPacket.value2 = data.Player_Turn[i].getHp();
					syncDataPacket.value3 = data.currentTurnIdx;
					syncDataPacket.value4 = (int)data.currentPhase;
					syncDataPacket.value5 = data.targetIdx;

					// ★【新方式】現在のフェーズに合わせて、cardIds配列(0?MAX)を安全に使い分ける
					if (data.currentPhase == BattlePhase::Select || data.currentPhase == BattlePhase::DefenseSelect) {
						for (int c = 0; c < MAX_HAND_CARD; ++c) {
							syncDataPacket.cardIds[c] = (c < (int)data.confirmedAttackCards.size()) ? data.confirmedAttackCards[c] : -1;
						}
					}
					else {
						for (int c = 0; c < MAX_HAND_CARD; ++c) {
							syncDataPacket.cardIds[c] = (c < (int)data.confirmedDefenseCards.size()) ? data.confirmedDefenseCards[c] : -1;
						}
					}

					netManager->BroadcastPacket(syncDataPacket);
				}

				// 2. 手札データの送信
				for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
					GamePacket handPacket;
					memset(&handPacket, 0, sizeof(GamePacket));

					handPacket.type = (int)CommandType::SYNC_PRIVATE_HAND;
					handPacket.value1 = i;

					const auto& handCards = data.Player_Turn[i].Hand.GetCards();
					for (int c = 0; c < MAX_HAND_CARD; ++c) {
						if (c < (int)handCards.size()) {
							handPacket.cardIds[c] = handCards[c].GetID();
						}
						else {
							handPacket.cardIds[c] = -1;
						}
					}
					netManager->BroadcastPacket(handPacket);
				}

				data.isChanged = false;
			}
		}
	}
	else {
		// オフライン（AI対戦）の従来処理
		if (myAction.hasAction) {
			if (myAction.isAttackDecision) {
				data.currentPhase = myAction.isHealAction ? BattlePhase::DefenseReveal : BattlePhase::DefenseSelect;
				localData.animFrame = 0;
			}
			else if (myAction.isDefenseDecision) {
				data.currentPhase = BattlePhase::DefenseReveal;
			}
		}
		aiManager.Update(data);
		logicManager.Update(data, localData);
	}

	return SceneName::BATTLE;
}

void BattleScene::Draw() const {
	if (localData.myPlayerIndex < 0) {
		DrawString(100, 100, "Waiting for Connection...", Col.GetBla());
		return;
	}

	// 画面全体のメインUI描画
	uiManager.Draw(data, localData);

	// デバッグ用情報の描画（不具合が完全に直ったらこの数行は消して大丈夫です）
	int whiteCol = GetColor(255, 255, 255);
	DrawFormatString(10, 150, whiteCol, "ONLINE_DEBUG: AtkCards=%d, DefCards=%d, Phase=%d",
		(int)data.confirmedAttackCards.size(),
		(int)data.confirmedDefenseCards.size(),
		(int)data.currentPhase);
}