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

	// カード初期配布
	if (!isOnline || (isOnline && netManager->IsHost())) {
		for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
			data.Player_Turn[i].Hand.Clear();
			for (int c = 0; c < START_CARD; ++c) {
				data.Player_Turn[i].Hand.Add(CardDB.GetRandomCard());
			}
			data.Player_Turn[i].Hand.Sort();
		}
	}

	// ホスト側：ID配布
	if (isOnline && netManager->IsHost()) {
		for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
			GamePacket idPacket;
			memset(&idPacket, 0, sizeof(GamePacket));
			idPacket.type = (int)CommandType::ASSIGN_ID;
			idPacket.value1 = i;
			netManager->SendPacketTo(i, idPacket);
		}
	}

	// クライアント側：初期は-1
	if (isOnline) {
		localData.myPlayerIndex = netManager->IsHost() ? 0 : -1;
	}
	else {
		localData.myPlayerIndex = 0;
	}

	data.currentTurnIdx = 0;
	data.currentPhase = BattlePhase::Select;
}

SceneName BattleScene::Update(const InputManager& input) {
	bool isOnline = (netManager != nullptr && netManager->IsConnected());

	if (isOnline) {
		GamePacket packet;
		while (netManager->PopPacket(packet)) {
			// パケット受信ログ
			printfDx("DEBUG: [Receive] Type: %d, Data: %d\n", packet.type, packet.value1);

			if (packet.type == (int)CommandType::ASSIGN_ID) {
				localData.myPlayerIndex = packet.value1;
			}

			// 【全員】フェーズの強制同期
			if (packet.type == (int)CommandType::SYNC_PHASE) {
				data.currentPhase = static_cast<BattlePhase>(packet.value1);
				localData.animFrame = 0;
			}

			// 【ホスト側】クライアントからのアクションを受信
			if (netManager->IsHost()) {
				if (packet.type == (int)CommandType::CLIENT_ACTION) {
					int clientId = packet.value3; // 送信元のプレイヤーID
					data.targetIdx = packet.value2;

					if (data.currentPhase == BattlePhase::Select) {
						data.confirmedAttackCards.clear();
						for (int c = 0; c < MAX_HAND_CARD; ++c) {
							if (packet.cardIds[c] != -1) {
								data.confirmedAttackCards.push_back(packet.cardIds[c]);
							}
						}
						data.currentPhase = BattlePhase::DefenseSelect;
						localData.animFrame = 0;
						data.isChanged = true;
					}
					else if (data.currentPhase == BattlePhase::DefenseSelect) {
						data.confirmedDefenseCards.clear();
						for (int c = 0; c < MAX_HAND_CARD; ++c) {
							if (packet.cardIds[c] != -1) {
								data.confirmedDefenseCards.push_back(packet.cardIds[c]);
							}
						}
						data.currentPhase = BattlePhase::DefenseReveal;
						localData.animFrame = 0;
						data.isChanged = true;
					}
				}
			}
			// 【クライアント側】ホストからのデータ同期を受信
			else {
				// 公開情報の同期（HP、ターン、フェーズ、場に出たカード）
				if (packet.type == (int)CommandType::SYNC_GAME_DATA) {
					int targetIdx = packet.value1;

					while ((int)data.Player_Turn.size() <= targetIdx) {
						data.Player_Turn.emplace_back(); // 箱がなければ作る
					}

					data.Player_Turn[targetIdx].setHp(packet.value2);
					data.currentTurnIdx = packet.value3;
					data.currentPhase = static_cast<BattlePhase>(packet.value4);
					data.targetIdx = packet.value5;

					// 場のカード状況の更新
					data.confirmedAttackCards.clear();
					for (int c = 0; c < 10; ++c) {
						int playedId = packet.playedCardIds[c];
						if (playedId != -1) data.confirmedAttackCards.push_back(playedId);
					}

					data.confirmedDefenseCards.clear();
					for (int c = 0; c < 10; ++c) {
						int playedId = packet.playedCardIds[10 + c];
						if (playedId != -1) data.confirmedDefenseCards.push_back(playedId);
					}
				}
				// 【クライアント側】秘密情報の同期（自分宛ての手札データのみ更新）
				else if (packet.type == (int)CommandType::SYNC_PRIVATE_HAND) {
					int targetIdx = packet.value1;

					// --- 【修正】: IDがまだ割り当てられていなくても、自分の手札パケットなら反映する ---
					// （もし localData.myPlayerIndex が -1 でも、将来自分のIDになる確率が高いパケットを捨てるのはもったいないため）
					// とりあえず ID が一致していれば読み込む、あるいはIDがまだ -1 ならとりあえずデータだけ読み込んでおく

					// 自分のIDがまだ未確定の場合、とりあえず受信した targetIdx を自分のIDとみなす（強引ですが初期化には有効）
					if (localData.myPlayerIndex == -1) {
						printfDx("DEBUG: [Auto-Assign] My ID was -1, assuming %d based on packet.\n", targetIdx);
						localData.myPlayerIndex = targetIdx;
					}

					if (targetIdx == localData.myPlayerIndex) {
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
						printfDx("DEBUG: [Success] Hand loaded for ID: %d\n", targetIdx);
					}
				}
			}
		}
	}

	// =============================================================
	// 2. プレイヤーデータ準備チェック（ガード節）
	// パケットを受信した上で、まだデータが揃っていなければ待機
	// =============================================================
	if (data.Player_Turn.empty()) {
		return SceneName::BATTLE;
	}
	// まだ自分のIDが割り当てられていない、または範囲外の場合は待機
	if (localData.myPlayerIndex < 0 || localData.myPlayerIndex >= (int)data.Player_Turn.size()) {
		return SceneName::BATTLE;
	}

	// =============================================================
	// 3. ローカルプレイヤーの入力処理
	// =============================================================
	Player& myPlayer = data.Player_Turn[localData.myPlayerIndex];
	bool isMyTurn = (data.currentTurnIdx == localData.myPlayerIndex);

	PlayerAction myAction = inputManager.Update(data, localData, input, myPlayer, localData.myPlayerIndex, isMyTurn);
	if (myAction.isSurrender) return SceneName::SELECT;

	// =============================================================
	// 4. アクション送信 / ホストのゲーム進行処理
	// =============================================================
	if (isOnline) {
		// クライアント側（またはホスト自身）のアクション送信処理
		if (myAction.hasAction) {
			if (netManager->IsHost()) {
				if (myAction.isAttackDecision) {
					data.currentPhase = myAction.isHealAction ? BattlePhase::DefenseReveal : BattlePhase::DefenseSelect;
					localData.animFrame = 0;
					data.isChanged = true;
				}
				else if (myAction.isDefenseDecision) {
					data.currentPhase = BattlePhase::DefenseReveal;
					localData.animFrame = 0;
					data.isChanged = true;
				}
			}
			else {
				if (myAction.isAttackDecision || myAction.isDefenseDecision) {
					GamePacket actionPacket;
					memset(&actionPacket, 0, sizeof(GamePacket));
					actionPacket.type = (int)CommandType::CLIENT_ACTION;
					actionPacket.value2 = myAction.targetIdx;
					actionPacket.value3 = localData.myPlayerIndex; // 自分が誰かをホストに申告

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

		// 【ホスト側】ゲーム状態の更新と同期
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

			if (data.currentPhase != oldPhase || data.currentTurnIdx != oldTurnIdx || data.targetIdx != oldTargetIdx) {
				data.isChanged = true;
			}
			for (size_t i = 0; i < data.Player_Turn.size(); ++i) {
				if (data.Player_Turn[i].getHp() != oldHps[i]) {
					data.isChanged = true;
				}
			}

			if (data.isChanged) {
				// ★ここから同期データの分離（重要）★

				// ① 公開情報（HPやフェーズなど）を全員にBroadcast
				for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
					GamePacket syncDataPacket;
					memset(&syncDataPacket, 0, sizeof(GamePacket));

					syncDataPacket.type = (int)CommandType::SYNC_GAME_DATA;
					syncDataPacket.value1 = i;
					syncDataPacket.value2 = data.Player_Turn[i].getHp();
					syncDataPacket.value3 = data.currentTurnIdx;
					syncDataPacket.value4 = (int)data.currentPhase;
					syncDataPacket.value5 = data.targetIdx;

					for (int c = 0; c < 10; ++c) {
						syncDataPacket.playedCardIds[c] = (c < (int)data.confirmedAttackCards.size()) ? data.confirmedAttackCards[c] : -1;
						syncDataPacket.playedCardIds[10 + c] = (c < (int)data.confirmedDefenseCards.size()) ? data.confirmedDefenseCards[c] : -1;
					}

					// 【デバッグ用】送信データの内容を確認
					printfDx("DEBUG: [Broadcast] Target: %d, HP: %d, Phase: %d\n", i, data.Player_Turn[i].getHp(), (int)data.currentPhase);

					netManager->BroadcastPacket(syncDataPacket);
				}

				// ② 秘密情報（各個人の手札）をそれぞれに個別送信 (Unicast)
				for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
					GamePacket handPacket;
					memset(&handPacket, 0, sizeof(GamePacket));

					handPacket.type = (int)CommandType::SYNC_PRIVATE_HAND;
					handPacket.value1 = i; // 誰宛ての手札か

					const auto& handCards = data.Player_Turn[i].Hand.GetCards();
					for (int c = 0; c < MAX_HAND_CARD; ++c) {
						if (c < (int)handCards.size()) {
							handPacket.cardIds[c] = handCards[c].GetID();
						}
						else {
							handPacket.cardIds[c] = -1;
						}
					}
					// SendPacketTo ではなく BroadcastPacket を使うことで送信抜けを完全に防ぐ
					netManager->BroadcastPacket(handPacket);
				}

				data.isChanged = false;
			}
		}
	}
	// =============================================================
	// オフライン（AI対戦）の処理
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

		aiManager.Update(data);
		logicManager.Update(data, localData);
	}

	return SceneName::BATTLE;
}

void BattleScene::Draw() const {
	// 【重要】待機状態の可視化
	if (localData.myPlayerIndex < 0) {
		// 画面に状況を表示（Col.GetBla()が未定義なら 0xffffff などに置き換えてください）
		DrawString(100, 100, "Waiting for Host Connection...", Col.GetBla());
		DrawFormatString(100, 130, Col.GetBla(), "Status: Searching for ID Packet...");
		return;
	}

	// 正常な描画
	uiManager.Draw(data, localData);
}