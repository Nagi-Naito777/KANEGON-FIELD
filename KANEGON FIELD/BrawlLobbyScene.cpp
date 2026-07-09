#include "BrawlLobbyScene.h"
#include "Picture.h"
#include "DxLib.h"

extern Player g_player;

std::vector<Player> BrawlLobbyScene::GetBattlePlayers() const {
    std::vector<Player> players;

    // 通信中かどうかを判定（メンバ変数 isConnected を利用）
    bool isOnline = isConnected;

    for (const auto& lobbyP : m_lobbyPlayers) {
        Player p;
        p.setName(lobbyP.name);

        // --- 修正ロジック ---
        if (lobbyP.name == g_player.getName()) {
            // 自分は常にHUMAN
            p.setControllerType(ControllerType::HUMAN);
        }
        else {
            // オンラインなら相手は通信相手(HUMAN)、オフラインならAI
            if (isOnline) {
                p.setControllerType(ControllerType::HUMAN);
            }
            else {
                p.setControllerType(ControllerType::AI);
            }
        }
        players.push_back(p);
    }

    printfDx("DEBUG: BrawlLobbyScene Returning %d players\n", (int)players.size());
    return players;
}

// 戻るボタンが押された時の処理（通信切断のロジックを追加）
SceneName BrawlLobbyScene::OnReturnClicked() {
    if (netManager && isConnected) {
        netManager->Disconnect(); // 通信を切断する
    }
    return SceneName::SELECT;
}

// バトル開始ボタンを表示するかどうか（ホスト限定のロジック）
bool BrawlLobbyScene::ShouldDrawStartButton() const {
    return isConnected && isHost;
}

BrawlLobbyScene::BrawlLobbyScene()
    : BaseLobbyScene(SelectScene::Option::PVP)
{
    for (int i = 0; i < MAX; i++) { isTeamState[i] = false; }
    for (int i = 0; i < MEMBER_MAX; i++) { isBattlePlayer[i] = false; }
}

void BrawlLobbyScene::SelectTeam(int teamId) {
    bool isAlreadySelected = false;
    if (teamId >= PVP && teamId <= TEAM_GREEN) {
        isAlreadySelected = isTeamState[teamId];
    }
    for (int i = PVP; i <= TEAM_GREEN; i++) { isTeamState[i] = false; }
    if (!isAlreadySelected && teamId >= PVP && teamId <= TEAM_GREEN) {
        isTeamState[teamId] = true;
    }
}


bool BrawlLobbyScene::CanStartBattle() const {
    if (m_lobbyPlayers.size() < 2) return false; // 2人以上いないと開始できない

    int firstTeam = m_lobbyPlayers[0].teamId;
    bool allSameTeam = true;

    for (const auto& p : m_lobbyPlayers) {
        if (p.teamId == PVP) return true; // 個人戦（乱闘）が1人でもいれば成立
        if (p.teamId != firstTeam) {
            allSameTeam = false; // 違うチームの人がいるので成立
            break;
        }
    }

    return !allSameTeam; // 全員同じチーム(true)なら false を返す
}

SceneName BrawlLobbyScene::Update(const InputManager& input) {

    // 通信更新
    if (netManager) {
        netManager->Update();
        isConnected = netManager->IsConnected();

        // 【ホスト専用】新しいクライアントが参加した瞬間の同期処理
        if (isConnected && isHost) {
            size_t currentClientCount = netManager->GetClientCount();
            if (currentClientCount > m_prevClientCount) {
                // 新しい人が入ってきた！ 最新のメンバー情報をその人に送る
                int newClientHandle = netManager->GetLastAddedClient();
                for (const auto& p : m_lobbyPlayers) {
                    GamePacket syncP;
                    syncP.type = (int)CommandType::SYNC_LOBBY;
                    syncP.teamId = p.teamId;
                    memset(syncP.playerName, 0, sizeof(syncP.playerName));
                    strncpy_s(syncP.playerName, sizeof(syncP.playerName), p.name.c_str(), _TRUNCATE);

                    netManager->SendPacketTo(newClientHandle, syncP);
                }
            }
            m_prevClientCount = currentClientCount; // 人数を更新
        }

        // 【受信処理の拡張】
        GamePacket packet;
        while (netManager->PopPacket(packet)) {
            // バトル開始の合図を受信
            if (packet.type == (int)CommandType::START_BATTLE) {
                return SceneName::BATTLE;
            }
            // 【クライアント専用】ロビー情報の同期を受信
            else if (packet.type == (int)CommandType::SYNC_LOBBY) {
                bool found = false;
                for (auto& p : m_lobbyPlayers) {
                    if (p.name == packet.playerName) {
                        p.teamId = packet.teamId;
                        found = true; break;
                    }
                }
                if (!found && m_lobbyPlayers.size() < MEMBER_MAX) {
                    m_lobbyPlayers.push_back({ packet.playerName, packet.teamId });
                }
            }
            // 誰かがチームを選択（またはキャンセル）した通知を受信
            else if (packet.type == (int)CommandType::SELECT_TEAM) {
                if (packet.teamId == -1) {
                    // teamId が -1 の場合はキャンセル扱い。リストからその人を削除する
                    for (auto it = m_lobbyPlayers.begin(); it != m_lobbyPlayers.end(); ) {
                        if (it->name == packet.playerName) {
                            it = m_lobbyPlayers.erase(it); // 削除
                        }
                        else {
                            ++it;
                        }
                    }
                }
                else {
                    // 名前がすでにリストにあるか探す（重複参加を防ぐ＆チーム変更対応）
                    bool found = false;
                    for (auto& p : m_lobbyPlayers) {
                        if (p.name == packet.playerName) {
                            p.teamId = packet.teamId;
                            found = true; break;
                        }
                    }
                    // 見つからなかったら、上限未満の場合のみ新規追加
                    if (!found && m_lobbyPlayers.size() < MEMBER_MAX) {
                        m_lobbyPlayers.push_back({ packet.playerName, packet.teamId });
                    }
                }
                // 【ホスト専用】受信した変更内容を、他のクライアント全員に中継する
                if (isHost) {
                    netManager->BroadcastPacket(packet);
                }
            }
            // 誰かとの接続が切れた通知を受信
            else if (packet.type == (int)CommandType::DISCONNECT) {
                for (auto it = m_lobbyPlayers.begin(); it != m_lobbyPlayers.end(); ) {
                    if (it->name == packet.playerName) {
                        it = m_lobbyPlayers.erase(it); // リストから削除
                    }
                    else {
                        ++it;
                    }
                    // 切断情報も他の人に中継する
                    if (isHost) {
                        netManager->BroadcastPacket(packet);
                    }
                }
            }
        }
    }
    for (int i = 0; i < MAX; i++) { isHoverIdx[i] = false; }

    // 戻るボタンのみ常に使用可能
    isHoverIdx[RETURN] = input.IsMouseOver(RET_BUT_X, RET_BUT_Y, RET_BUTW, RET_BUTH);

    if (isConnected) {
        // 参加者全員の状態を見て、現在のロビーの「モード」を判定する
        bool isSoloMode = false;
        bool isTeamMode = false;
        for (const auto& p : m_lobbyPlayers) {
            if (p.teamId == PVP) isSoloMode = true; // 誰か一人でも個人戦を選んだらソロモード
            if (p.teamId >= TEAM_RED && p.teamId <= TEAM_GREEN) isTeamMode = true; // 誰か一人でもチームを選んだらチームモード
        }

        int btnW = 200, btnH = 100, startX = 750;

        if (ShouldDrawStartButton()) {
            isHoverIdx[BATTLE_START] = input.IsMouseOver(350, 550, 300, 150);
        }

        // 判定した全体モードをもとに、ホバー判定をロックする
        for (int i = PVP; i <= TEAM_GREEN; i++) {
            if (i >= TEAM_RED && i <= TEAM_GREEN && !isSoloMode) {
                // ソロモードでなければ、チームボタンを押せる
                isHoverIdx[i] = input.IsMouseOver(startX, 100 + ((i - TEAM_RED) * 104), btnW, btnH);
            }
            else if (i == PVP && !isTeamMode) {
                // チームモードでなければ、ソロボタンを押せる
                isHoverIdx[i] = input.IsMouseOver(50, 100, btnW, btnH);
            }
        }
    }
    else {
        // 未接続時のホスト・クライアントボタン判定（変更なし）
        const int HOST_X = 200, HOST_Y = 300, HOST_W = 200, HOST_H = 100;
        const int CLIENT_X = 450, CLIENT_Y = 300, CLIENT_W = 200, CLIENT_H = 100;
        isHoverIdx[BTN_HOST] = input.IsMouseOver(HOST_X, HOST_Y, HOST_W, HOST_H);
        isHoverIdx[BTN_CLIENT] = input.IsMouseOver(CLIENT_X, CLIENT_Y, CLIENT_W, CLIENT_H);
    }

    // クリック処理
    if (input.IsLeftClicked()) {
        // オーバーライドした関数を呼ぶ
        if (isHoverIdx[RETURN]) return OnReturnClicked();

        // 通信状態によって処理を分岐
        if (isConnected) {
            // ShouldDrawStartButton() を使ってクリック判定を行う
            if (ShouldDrawStartButton() && isHoverIdx[BATTLE_START]) {
                if (CanStartBattle()) {
                    GamePacket p; p.type = (int)CommandType::START_BATTLE;

                    // ホストがバトル開始を決定するので BroadcastPacket を使う
                    netManager->BroadcastPacket(p);
                    return SceneName::BATTLE;
                }
                else {
                    printfDx("チームが偏っているか、人数が不足しています\n");
                }
            }

            // チーム選択＆キャンセルロジック
            for (int i = PVP; i <= TEAM_GREEN; i++) {
                if (isHoverIdx[i]) {
                    int sendTeamId = i; // 相手に送るチームID
                    bool isCancel = false;

                    // 1. 自分の画面のリストをチェックし、キャンセルの判定をする
                    for (auto it = m_lobbyPlayers.begin(); it != m_lobbyPlayers.end(); ) {
                        if (it->name == g_player.getName()) {
                            if (it->teamId == i) {
                                // すでに同じチームを選択していたらキャンセル（リストから削除）
                                it = m_lobbyPlayers.erase(it);
                                sendTeamId = -1; // -1をキャンセルの合図にする
                                isCancel = true;
                            }
                            else {
                                // 別のチームへの変更
                                it->teamId = i;
                                ++it;
                            }
                        }
                        else {
                            ++it;
                        }
                    }

                    // 2. キャンセルではなく、かつまだリストに自分がいない場合は新規追加
                    if (!isCancel && sendTeamId != -1) {
                        bool found = false;
                        for (const auto& p : m_lobbyPlayers) {
                            if (p.name == g_player.getName()) { found = true; break; }
                        }
                        if (!found && m_lobbyPlayers.size() < MEMBER_MAX) {
                            m_lobbyPlayers.push_back({ g_player.getName(), sendTeamId });
                        }
                    }

                    // 3. 通信で全員に知らせる
                    GamePacket p;
                    p.type = (int)CommandType::SELECT_TEAM;
                    p.teamId = sendTeamId; // チーム変更時はそのID、キャンセル時は -1 が入る

                    // memset でゼロクリア（ここはそのまま）
                    memset(p.playerName, 0, sizeof(p.playerName));

                    // strncpy_s に変更し、引数を4つにする（_TRUNCATE を付けるのがポイント！）
                    strncpy_s(p.playerName, sizeof(p.playerName), g_player.getName().c_str(), _TRUNCATE);

                    // ★変更：ホストとクライアントで送信方法を分ける
                    if (isHost) {
                        netManager->BroadcastPacket(p); // ホストは他の全員に配る
                    }
                    else {
                        netManager->SendPacket(p);      // クライアントはホストに送る
                    }
                }
            }
        }
        else {
            if (netManager != nullptr) {
                if (isHoverIdx[BTN_HOST]) {
                    isHost = true; // 自分がホストであることを記憶
                    netManager->StartHost(9850);
                    printfDx("ホスト待機を開始しました\n");
                }
                if (isHoverIdx[BTN_CLIENT]) {
                    isHost = false; // クライアントであることを記憶
                    netManager->ConnectAsClient("127.0.0.1", 9850);
                    printfDx("接続を試みます...\n");
                }
            }
        }
    }

    return SceneName::SETTING;
}

void BrawlLobbyScene::DrawSpecificUI() const {
    int startX = 750;
    // 描画時も「m_lobbyPlayers」の内容から現在のモードを判定する
    bool isSoloSelected = false;
    bool isAnyTeamSelected = false;
    for (const auto& p : m_lobbyPlayers) {
        if (p.teamId == PVP) isSoloSelected = true;
        if (p.teamId >= TEAM_RED && p.teamId <= TEAM_GREEN) isAnyTeamSelected = true;
    }

    // チームボタン描画
    for (int i = TEAM_RED; i <= TEAM_GREEN; i++) {
        int num = i - TEAM_RED;
        int btnY = 100 + (num * 104);
        // 個人戦が選ばれていたらチームボタンはロック
        if (isSoloSelected) {
            Pic.ButtonRockDraw(startX, btnY, Pic.GetTeamButton(num));
        }
        else {
            if (isHoverIdx[i]) { Pic.MouseHoverDraw(startX, btnY + 1, Pic.GetTeamButton(num)); }
            else { DrawGraph(startX, btnY, Pic.GetTeamButton(num), TRUE); }
        }
    }

    // 個人戦ボタン描画
    if (isAnyTeamSelected) { Pic.ButtonRockDraw(50, 100, Pic.GetSoloButton()); }
    else {
        if (isHoverIdx[PVP]) { Pic.MouseHoverDraw(50, 100 + 1, Pic.GetSoloButton()); }
        else { DrawGraph(50, 100, Pic.GetSoloButton(), TRUE); }
    }

    // 参加順（vectorの配列順）で上から下に描画する
    int startY = 80; // 一番上の白枠のY座標（画像に合わせて微調整してください）
    int stepY = 45;  // 枠と枠の縦の間隔（画像に合わせて微調整してください）

    for (int j = 0; j < MEMBER_MAX; j++) {
        int drawY = startY + (j * stepY); // jが増えるごとに下にズレる

        if (j < m_lobbyPlayers.size()) {
            // リストに人がいる場合は、その人のチーム色で描画
            unsigned int drawColor = Col.GetWhi();
            if (m_lobbyPlayers[j].teamId == TEAM_RED)    drawColor = Col.GetRed();
            if (m_lobbyPlayers[j].teamId == TEAM_BLUE)   drawColor = Col.GetBlu();
            if (m_lobbyPlayers[j].teamId == TEAM_YELLOW) drawColor = Col.GetYel();
            if (m_lobbyPlayers[j].teamId == TEAM_GREEN)  drawColor = Col.GetGre();
            if (m_lobbyPlayers[j].teamId == PVP)         drawColor = Col.GetWhi();

            DrawPlayerTeam(m_lobbyPlayers[j].name.c_str(), drawY, drawColor);
        }
        else {
            // リストの人数以上のインデックスは、空の白枠として描画
            DrawPlayerTeam(_T(""), drawY, Col.GetWhi());
        }
    }

    // --- ホバー判定エリアの定義 ---
    const int HOST_X = 200, HOST_Y = 300, HOST_W = 200, HOST_H = 100;
    const int CLIENT_X = 450, CLIENT_Y = 300, CLIENT_W = 200, CLIENT_H = 100;

    // --- 【通信状態の表示】 ---
    if (!isConnected) {
        // 背景を薄暗くする（画面全体を覆う）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150); // 150は透明度（0-255）
        DrawBox(START_X, START_Y + 50, WIN_MAX_X, WIN_MAX_Y - 50, GetColor(0, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0); // 元に戻す

        // ボタンの座標とサイズ
        const int HOST_X = 200, HOST_Y = 300, W = 200, H = 100;
        const int CLIENT_X = 450, CLIENT_Y = 300;

        // --- ホストボタンの描画 ---
        unsigned int hostColor = isHoverIdx[BTN_HOST] ? Col.GetGra() : Col.GetGre();
        DrawBox(HOST_X, HOST_Y, HOST_X + W, HOST_Y + H, hostColor, TRUE);
        // 白い枠線
        DrawBox(HOST_X, HOST_Y, HOST_X + W, HOST_Y + H, Col.GetWhi(), FALSE);
        DrawString(HOST_X + 60, HOST_Y + 40, "HOST", Col.GetBla());

        // --- クライアントボタンの描画 ---
        unsigned int clientColor = isHoverIdx[BTN_CLIENT] ? Col.GetGra() : Col.GetRed();
        DrawBox(CLIENT_X, CLIENT_Y, CLIENT_X + W, CLIENT_Y + H, clientColor, TRUE);
        // 白い枠線
        DrawBox(CLIENT_X, CLIENT_Y, CLIENT_X + W, CLIENT_Y + H, Col.GetWhi(), FALSE);
        DrawString(CLIENT_X + 40, CLIENT_Y + 40, "CLIENT", Col.GetBla());
    }
    else {
        // 接続後は「接続中」を表示
        // ここでも背景を少し暗くすると文字が読みやすくなります
        DrawBox(50, 600, 260, 660, Col.GetWhi(), TRUE); // 文字の背景を白に
        DrawBox(50, 600, 260, 660, Col.GetBla(), FALSE); // 黒い枠
        DrawString(75, 620, "接続待機中...", Col.GetBla());
    }
}