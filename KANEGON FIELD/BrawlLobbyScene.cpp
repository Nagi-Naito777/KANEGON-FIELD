#include "BrawlLobbyScene.h"
#include "Picture.h"
#include "DxLib.h"

extern Player g_player;

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

SceneName BrawlLobbyScene::Update(const InputManager& input) {

    // 通信更新
    if (netManager) {
        netManager->Update();
        isConnected = netManager->IsConnected();

        // 【受信】ホストから「バトル開始」パケットが来たら画面遷移する
        GamePacket packet;
        while (netManager->PopPacket(packet)) {
            if (packet.type == CommandType::START_BATTLE) {
                return SceneName::BATTLE;
            }
        }
    }

    int btnW = 200, btnH = 100, startX = 750;
    for (int i = 0; i < MAX; i++) { isHoverIdx[i] = false; }

    // 戻るボタンのみ常に使用可能
    isHoverIdx[RETURN] = input.IsMouseOver(RET_BUT_X, RET_BUT_Y, RET_BUTW, RET_BUTH);

    // 通信状態に応じたボタン判定の振り分け
    if (isConnected) {
        // --- 接続済み：ロビー機能（チーム選択・バトル開始） ---
        int btnW = 200, btnH = 100, startX = 750;

        isHoverIdx[BATTLE_START] = input.IsMouseOver(350, 550, 300, 150);

        bool isSoloSelected = isTeamState[PVP];
        bool isAnyTeamSelected = isTeamState[TEAM_RED] || isTeamState[TEAM_BLUE] || isTeamState[TEAM_YELLOW] || isTeamState[TEAM_GREEN];

        // 各種チームボタンのホバー判定
        for (int i = PVP; i <= TEAM_GREEN; i++) {
            if (i >= TEAM_RED && i <= TEAM_GREEN && !isSoloSelected) {
                isHoverIdx[i] = input.IsMouseOver(startX, 100 + ((i - TEAM_RED) * 104), btnW, btnH);
            }
            else if (i == PVP && !isAnyTeamSelected) {
                isHoverIdx[i] = input.IsMouseOver(50, 100, btnW, btnH);
            }
        }
    }
    else {
        // --- 未接続：接続機能（ホスト・クライアント） ---
        const int HOST_X = 200, HOST_Y = 300, HOST_W = 200, HOST_H = 100;
        const int CLIENT_X = 450, CLIENT_Y = 300, CLIENT_W = 200, CLIENT_H = 100;

        isHoverIdx[BTN_HOST] = input.IsMouseOver(HOST_X, HOST_Y, HOST_W, HOST_H);
        isHoverIdx[BTN_CLIENT] = input.IsMouseOver(CLIENT_X, CLIENT_Y, CLIENT_W, CLIENT_H);
    }

    // クリック処理
    if (input.IsLeftClicked()) {
        // 「戻る」は常に反応
        if (isHoverIdx[RETURN]) return SceneName::SELECT;

        // 通信状態によって処理を分岐
        if (isConnected) {
            // 接続済み：ロビー操作のみ受付
            if (isHoverIdx[BATTLE_START]) return SceneName::BATTLE;

            for (int i = PVP; i <= TEAM_GREEN; i++) {
                if (isHoverIdx[i]) {
                    SelectTeam(i);
                    isBattlePlayer[0] = isTeamState[i];
                }
            }
        }
        else {
            // 未接続：接続操作のみ受付
            if (netManager != nullptr) {
                if (isHoverIdx[BTN_HOST]) {
                    netManager->StartHost(9850);
                    printfDx("ホスト待機を開始しました\n");
                }
                if (isHoverIdx[BTN_CLIENT]) {
                    netManager->ConnectAsClient("127.0.0.1", 9850);
                    printfDx("接続を試みます...\n");
                }
            }
            else {
                printfDx("エラー: netManager が初期化されていません！\n");
            }
        }
    }

    return SceneName::SETTING;
}

void BrawlLobbyScene::DrawSpecificUI() const {
    int startX = 750;
    bool isSoloSelected = isTeamState[PVP];
    bool isAnyTeamSelected = isTeamState[TEAM_RED] || isTeamState[TEAM_BLUE] || isTeamState[TEAM_YELLOW] || isTeamState[TEAM_GREEN];

    // チームボタン描画
    for (int i = TEAM_RED; i <= TEAM_GREEN; i++) {
        int num = i - TEAM_RED;
        int btnY = 100 + (num * 104);
        if (isSoloSelected) { Pic.ButtonRockDraw(startX, btnY, Pic.GetTeamButton(num)); }
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

    // 所属チームカラーに応じたプレイヤー枠描画
    unsigned int myColor = Col.GetWhi();
    if (isTeamState[TEAM_RED])    myColor = Col.GetRed();
    if (isTeamState[TEAM_BLUE])   myColor = Col.GetBlu();
    if (isTeamState[TEAM_YELLOW]) myColor = Col.GetYel();
    if (isTeamState[TEAM_GREEN])  myColor = Col.GetGre();

    for (int j = 0; j < MEMBER_MAX; j++) {
        if (isBattlePlayer[j]) {
            unsigned int drawColor = (j == 0) ? myColor : Col.GetWhi();
            DrawPlayerTeam(g_player.getName(), 100 + (j * 40), drawColor);
        }
        else {
            DrawPlayerTeam(_T(" "), 100 + (j * 40), Col.GetWhi());
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
        DrawBox(295, 295, 505, 355, Col.GetBla(), TRUE); // 文字の背景を黒に
        DrawBox(295, 295, 505, 355, Col.GetWhi(), FALSE); // 白い枠
        DrawString(320, 315, "接続待機中...", Col.GetYel());
    }
}