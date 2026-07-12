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
    prevHps.clear(); // HP履歴も初期化

    // 外部からちゃんとプレイヤーが渡された時だけ上書きする
    if (!initialPlayers.empty()) {
        data.Player_Turn = initialPlayers;

        // 現在のHPで初期化
        prevHps.resize(initialPlayers.size());
        for (size_t i = 0; i < initialPlayers.size(); ++i) {
            prevHps[i] = initialPlayers[i].getHp();
        }
    }

	// オンラインかどうかを判定
	bool isOnline = (netManager != nullptr && netManager->IsConnected());
	printfDx("DEBUG: netManager pointer: %p, isConnected: %d\n", (void*)netManager, (int)(netManager ? netManager->IsConnected() : -1));

	// --- AI対戦（オフライン）の時だけ実行する処理 ---
    if (!isOnline) {
        SetupOfflineAI();
    }

    DistributeInitialCards(isOnline);
    AssignPlayerIDs(isOnline);

    data.currentTurnIdx = 0;
    data.currentPhase = BattlePhase::Select;

    if (isOnline && netManager->IsHost()) {
        data.isChanged = true;
    }
}

void BattleScene::SetupOfflineAI() {
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

    std::shuffle(aiNames.begin(), aiNames.end(), engine);

    int aiNameIdx = 0;
    for (auto& p : data.Player_Turn) {
        if (p.getControllerType() == ControllerType::AI) {
            std::string candidate = aiNames[aiNameIdx];
            aiNameIdx++;
            if (candidate == g_player.getName() && aiNameIdx < (int)aiNames.size()) {
                candidate = aiNames[aiNameIdx];
                aiNameIdx++;
            }
            p.setName(candidate);
        }
    }
    std::shuffle(data.Player_Turn.begin(), data.Player_Turn.end(), engine);
}

void BattleScene::DistributeInitialCards(bool isOnline) {
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
}

void BattleScene::AssignPlayerIDs(bool isOnline) {
    if (isOnline) {
        if (netManager->IsHost()) {
            localData.myPlayerIndex = 0;
            printfDx("DEBUG: [INIT] I am Host. Assigned ID: 0\n");
        }
        else {
            if (localData.myPlayerIndex < 0) {
                localData.myPlayerIndex = 1; // ロビー処理移行時までの暫定
            }
            printfDx("DEBUG: [INIT] I am Client. My ID: %d\n", localData.myPlayerIndex);
        }
    }
    else {
        localData.myPlayerIndex = 0;
        for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
            if (data.Player_Turn[i].getControllerType() == ControllerType::HUMAN) {
                localData.myPlayerIndex = i;
                break;
            }
        }
    }
}

// ==========================================
// 更新処理
// ==========================================
SceneName BattleScene::Update(const InputManager& input) {
    bool isOnline = (netManager != nullptr && netManager->IsConnected());

    // 1. ネットワーク受信処理
    if (isOnline) {
        netManager->Update();
        HandleNetworkReceive();
    }

    // データ未準備なら待機
    if (data.Player_Turn.empty() || localData.myPlayerIndex < 0 || localData.myPlayerIndex >= (int)data.Player_Turn.size()) {
        return SceneName::BATTLE;
    }

    // ホスト/クライアントに関わらず、必ずUIイベント（ポップアップ監視）を通す
    // クライアントだけでなく、ホスト自身の画面でもHP監視を行う必要があるため
    UpdateClientUIEvents();

    // 3. ローカルの入力処理
    Player& myPlayer = data.Player_Turn[localData.myPlayerIndex];
    bool isMyTurn = (data.currentTurnIdx == localData.myPlayerIndex);
    PlayerAction myAction = inputManager.Update(data, localData, input, myPlayer, localData.myPlayerIndex, isMyTurn);

    if (myAction.isSurrender) return SceneName::SELECT;

    // 4. アクションの反映（送信または直接適用）
    ProcessPlayerAction(myAction, isOnline);

    // 5. ゲームロジック進行と同期送信（ホストまたはオフライン時）
    if (!isOnline || netManager->IsHost()) {
        UpdateHostLogicAndSync();
    }

    return SceneName::BATTLE;
}

// ==========================================
// リファクタリング関数群 (Update内部処理)
// ==========================================
void BattleScene::HandleNetworkReceive() {
    GamePacket packet;
    while (netManager->PopPacket(packet)) {
        if (packet.type == (int)CommandType::SYNC_PHASE) {
            data.currentPhase = static_cast<BattlePhase>(packet.value1);
            localData.animFrame = 0;
        }

        if (netManager->IsHost() && packet.type == (int)CommandType::CLIENT_ACTION) {
            ProcessClientAction(packet);
        }
        else if (!netManager->IsHost()) {
            ProcessHostSyncData(packet);
        }
    }
}

void BattleScene::ProcessClientAction(const GamePacket& packet) {
    int clientId = packet.value3;
    if (clientId < 0 || clientId >= (int)data.Player_Turn.size()) return;

    // クライアントの行動なので、ホストが持っているそのクライアントの手札を参照する
    Player& clientPlayer = data.Player_Turn[clientId];
    const auto& handCards = clientPlayer.Hand.GetCards();

    if (data.currentPhase == BattlePhase::Select && clientId == data.currentTurnIdx) {
        data.targetIdx = packet.value2;
        data.confirmedAttackCards.clear();
        for (int c = 0; c < MAX_HAND_CARD; ++c) {
            int handIdx = packet.playedCardIds[c];
            // 手札のインデックスとして妥当かチェックしてから実体を保存
            if (handIdx != -1 && handIdx >= 0 && handIdx < (int)handCards.size()) {
                data.confirmedAttackCards.push_back(handCards[handIdx]);
            }
        }

        // ヒール行動だった場合、DefenseSelectを飛ばす
        bool isHeal = (packet.value1 == 1);
        data.currentPhase = isHeal ? BattlePhase::DefenseReveal : BattlePhase::DefenseSelect;

        localData.animFrame = 0;
        data.isChanged = true;
    }
    else if (data.currentPhase == BattlePhase::DefenseSelect && clientId == data.targetIdx) {
        data.confirmedDefenseCards.clear();
        for (int c = 0; c < MAX_HAND_CARD; ++c) {
            int handIdx = packet.playedCardIds[c];
            if (handIdx != -1 && handIdx >= 0 && handIdx < (int)handCards.size()) {
                data.confirmedDefenseCards.push_back(handCards[handIdx]);
            }
        }
        data.currentPhase = BattlePhase::DefenseReveal;
        localData.animFrame = 0;
        data.isChanged = true;
    }
}

void BattleScene::ProcessHostSyncData(const GamePacket& packet) {
    if (packet.type == (int)CommandType::SYNC_GAME_DATA) {
        int targetIdx = packet.value1;
        while ((int)data.Player_Turn.size() <= targetIdx) {
            data.Player_Turn.emplace_back();
            prevHps.push_back(0);
        }

        data.Player_Turn[targetIdx].setHp(packet.value2);
        data.currentTurnIdx = packet.value3;

        BattlePhase newPhase = static_cast<BattlePhase>(packet.value4);
        if (data.currentPhase != newPhase) {
            data.currentPhase = newPhase;
            localData.animFrame = 0;

            // フェーズがホスト主導で切り替わった場合、クライアントの選択UIもリセットする
            localData.localSelectingCards.clear();
            localData.localTargetIdx = -1;
        }

        data.targetIdx = packet.value5;

        // 【修正】パケットから流れてくるカードID(固有ID)をもとに、CardDBから実体を取得して再構築
        data.confirmedAttackCards.clear();
        data.confirmedDefenseCards.clear();
        for (int c = 0; c < MAX_HAND_CARD; ++c) {
            if (packet.playedCardIds[c] != -1) {
                data.confirmedAttackCards.push_back(CardDB.GetCardByID(packet.playedCardIds[c]));
            }
            if (packet.cardIds[c] != -1) {
                data.confirmedDefenseCards.push_back(CardDB.GetCardByID(packet.cardIds[c]));
            }
        }
    }
    else if (packet.type == (int)CommandType::SYNC_PRIVATE_HAND) {
        int targetIdx = packet.value1;
        if (targetIdx >= 0) {
            // 手札非表示バグの解消
            if (localData.myPlayerIndex != targetIdx) {
                localData.myPlayerIndex = targetIdx;
            }

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

void BattleScene::UpdateClientUIEvents() {
    // 1. プレイヤー数が変更された場合の初期化
    if (prevHps.size() != data.Player_Turn.size()) {
        prevHps.resize(data.Player_Turn.size());
        for (size_t i = 0; i < data.Player_Turn.size(); ++i) {
            prevHps[i] = data.Player_Turn[i].getHp();
        }
        return;
    }

    // 2. タイマー減少処理（前述の通り）
    auto& popups = localData.popups;
    for (auto it = popups.begin(); it != popups.end(); ) {
        it->timer--;
        if (it->timer <= 0) it = popups.erase(it);
        else ++it;
    }

    // 3. HP変動の検知とポップアップ生成
    for (size_t i = 0; i < data.Player_Turn.size(); ++i) {
        int currentHp = data.Player_Turn[i].getHp();

        // 差分が発生している場合のみ処理
        if (currentHp != prevHps[i]) {
            int diff = prevHps[i] - currentHp;

            if (diff > 0) {
                // メインダメージ：GetPopupTextを使用して文字列を取得
                localData.popups.emplace_back(PopupType::Damage, (int)i, UIHelper::GetPopupText(PopupType::Damage, diff), 60, 0);

                // 闇属性の追加ダメージ
                if (data.isLastAttackDark) {
                    int yamiDama = (int)(diff * 0.5f);
                    // ここもGetPopupTextを通す
                    localData.popups.emplace_back(PopupType::YamiDama, (int)i, UIHelper::GetPopupText(PopupType::YamiDama, yamiDama), 60, 30);

                    data.isLastAttackDark = false; // 表示したらリセット
                }
            }
            else if (diff < 0) {
                // 回復：GetPopupTextを使用
                localData.popups.emplace_back(PopupType::Heal, (int)i, UIHelper::GetPopupText(PopupType::Heal, -diff), 60, 0);
            }
            prevHps[i] = currentHp;
        }
    }
}

void BattleScene::ProcessPlayerAction(const PlayerAction& myAction, bool isOnline) {
    if (!myAction.hasAction) return;

    // 自キャラの手札を取得 (UIが渡してくるのは手札のインデックスなので、ここから実体を引く)
    Player& myPlayer = data.Player_Turn[localData.myPlayerIndex];
    const auto& handCards = myPlayer.Hand.GetCards();

    if (isOnline) {
        if (netManager->IsHost()) {
            if (myAction.isAttackDecision) {
                data.confirmedAttackCards.clear();
                for (int idx : myAction.selectedCardIdxs) {
                    if (idx >= 0 && idx < (int)handCards.size()) {
                        data.confirmedAttackCards.push_back(handCards[idx]); // 【修正】インデックスから実体を登録
                    }
                }
                data.currentPhase = myAction.isHealAction ? BattlePhase::DefenseReveal : BattlePhase::DefenseSelect;
                localData.animFrame = 0;
                data.isChanged = true;

                localData.localSelectingCards.clear();
                localData.localTargetIdx = -1;
            }
            else if (myAction.isDefenseDecision) {
                data.confirmedDefenseCards.clear();
                for (int idx : myAction.selectedCardIdxs) {
                    if (idx >= 0 && idx < (int)handCards.size()) {
                        data.confirmedDefenseCards.push_back(handCards[idx]); // 【修正】インデックスから実体を登録
                    }
                }
                data.currentPhase = BattlePhase::DefenseReveal;
                localData.animFrame = 0;
                data.isChanged = true;

                localData.localSelectingCards.clear();
                localData.localTargetIdx = -1;
            }
        }
        else {
            if (myAction.isAttackDecision || myAction.isDefenseDecision) {
                GamePacket actionPacket{};
                actionPacket.type = (int)CommandType::CLIENT_ACTION;
                actionPacket.value1 = myAction.isHealAction ? 1 : 0;
                actionPacket.value2 = myAction.targetIdx;
                actionPacket.value3 = localData.myPlayerIndex;

                for (int c = 0; c < MAX_HAND_CARD; ++c) {
                    actionPacket.playedCardIds[c] = -1;
                    actionPacket.cardIds[c] = -1;
                }

                // 【ここは変更なし】クライアントは「手札の何番目を選んだか(インデックス)」をホストに送信する
                for (size_t c = 0; c < myAction.selectedCardIdxs.size() && c < MAX_HAND_CARD; ++c) {
                    actionPacket.playedCardIds[c] = myAction.selectedCardIdxs[c];
                }

                netManager->SendPacket(actionPacket);

                localData.localSelectingCards.clear();
                localData.localTargetIdx = -1;
            }
        }
    }
    else {
        // オフライン処理
        if (myAction.isAttackDecision) {
            data.confirmedAttackCards.clear();
            for (int idx : myAction.selectedCardIdxs) {
                if (idx >= 0 && idx < (int)handCards.size()) {
                    data.confirmedAttackCards.push_back(handCards[idx]); // 【修正】
                }
            }
            data.currentPhase = myAction.isHealAction ? BattlePhase::DefenseReveal : BattlePhase::DefenseSelect;
            localData.animFrame = 0;

            localData.localSelectingCards.clear();
            localData.localTargetIdx = -1;
        }
        else if (myAction.isDefenseDecision) {
            data.confirmedDefenseCards.clear();
            for (int idx : myAction.selectedCardIdxs) {
                if (idx >= 0 && idx < (int)handCards.size()) {
                    data.confirmedDefenseCards.push_back(handCards[idx]); // 【修正】
                }
            }
            data.currentPhase = BattlePhase::DefenseReveal;
            localData.localSelectingCards.clear();
            localData.localTargetIdx = -1;
        }
    }
}

void BattleScene::UpdateHostLogicAndSync() {
    BattlePhase oldPhase = data.currentPhase;
    int oldTurnIdx = data.currentTurnIdx;
    int oldTargetIdx = data.targetIdx;

    std::vector<int> oldHps(data.Player_Turn.size());
    for (size_t i = 0; i < data.Player_Turn.size(); ++i) oldHps[i] = data.Player_Turn[i].getHp();

    logicManager.Update(data, localData);
    aiManager.Update(data);

    if (data.currentPhase != oldPhase || data.currentTurnIdx != oldTurnIdx || data.targetIdx != oldTargetIdx) {
        data.isChanged = true;
    }
    for (size_t i = 0; i < data.Player_Turn.size(); ++i) {
        if (data.Player_Turn[i].getHp() != oldHps[i]) data.isChanged = true;
    }

    static int autoSyncTimer = 0;
    autoSyncTimer++;
    if (autoSyncTimer >= 30) {
        data.isChanged = true;
        autoSyncTimer = 0;
    }

    // 通信対戦時のみ同期パケット送信
    if (netManager != nullptr && netManager->IsConnected() && data.isChanged) {

        // 1. 基本ゲームデータと「場に出たカード」の送信
        for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
            GamePacket syncDataPacket{};
            syncDataPacket.type = (int)CommandType::SYNC_GAME_DATA;
            syncDataPacket.value1 = i;
            syncDataPacket.value2 = data.Player_Turn[i].getHp();
            syncDataPacket.value3 = data.currentTurnIdx;
            syncDataPacket.value4 = (int)data.currentPhase;
            syncDataPacket.value5 = data.targetIdx;

            for (int c = 0; c < MAX_HAND_CARD; ++c) {
                syncDataPacket.playedCardIds[c] = -1;
                syncDataPacket.cardIds[c] = -1;
            }

            // 【修正】配列内の実体から .GetID() を使って固有IDを取得し、パケットに格納する
            for (size_t c = 0; c < data.confirmedAttackCards.size() && c < MAX_HAND_CARD; ++c) {
                syncDataPacket.playedCardIds[c] = data.confirmedAttackCards[c].GetID();
            }
            for (size_t c = 0; c < data.confirmedDefenseCards.size() && c < MAX_HAND_CARD; ++c) {
                syncDataPacket.cardIds[c] = data.confirmedDefenseCards[c].GetID();
            }
            netManager->BroadcastPacket(syncDataPacket);
        }

        // 2. 手札データの送信
        const auto& clientHandles = netManager->GetClientHandles();
        for (int i = 0; i < (int)data.Player_Turn.size(); ++i) {
            GamePacket handPacket{};
            handPacket.type = (int)CommandType::SYNC_PRIVATE_HAND;
            handPacket.value1 = i;

            for (int c = 0; c < MAX_HAND_CARD; ++c) {
                handPacket.cardIds[c] = -1;
                handPacket.playedCardIds[c] = -1;
            }

            const auto& handCards = data.Player_Turn[i].Hand.GetCards();
            for (size_t c = 0; c < handCards.size() && c < MAX_HAND_CARD; ++c) {
                handPacket.cardIds[c] = handCards[c].GetID();
            }

            if (i > 0 && (i - 1) < (int)clientHandles.size()) {
                netManager->SendPacketTo(clientHandles[i - 1], handPacket);
            }
        }
        data.isChanged = false;
    }
}

// ==========================================
// 描画処理 
// ==========================================
void BattleScene::Draw() const {
    if (localData.myPlayerIndex < 0) {
        //DrawString(100, 100, "Waiting for Connection...", Col.GetBla());
        return;
    }

    uiManager.Draw(data, localData);

    DrawFormatString(10, 150, Col.GetBla(), "ONLINE_DEBUG: AtkCards=%d, DefCards=%d, Phase=%d",
        (int)data.confirmedAttackCards.size(),
        (int)data.confirmedDefenseCards.size(),
        (int)data.currentPhase);
}