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
    int btnW = 200, btnH = 100, startX = 750;
    for (int i = 0; i < MAX; i++) { isHoverIdx[i] = false; }

    isHoverIdx[RETURN] = input.IsMouseOver(RET_BUT_X, RET_BUT_Y, RET_BUTW, RET_BUTH);
    isHoverIdx[BATTLE_START] = input.IsMouseOver(350, 550, 300, 150);

    bool isSoloSelected = isTeamState[PVP];
    bool isAnyTeamSelected = isTeamState[TEAM_RED] || isTeamState[TEAM_BLUE] || isTeamState[TEAM_YELLOW] || isTeamState[TEAM_GREEN];

    // 各種チームボタンのホバー
    for (int i = PVP; i <= TEAM_GREEN; i++) {
        if (i >= TEAM_RED && i <= TEAM_GREEN && !isSoloSelected) {
            isHoverIdx[i] = input.IsMouseOver(startX, 100 + ((i - TEAM_RED) * 104), btnW, btnH);
        }
        else if (i == PVP && !isAnyTeamSelected) {
            isHoverIdx[i] = input.IsMouseOver(50, 100, btnW, btnH);
        }
    }

    if (input.IsLeftClicked()) {
        if (isHoverIdx[RETURN]) return SceneName::SELECT;
        if (isHoverIdx[BATTLE_START]) return SceneName::BATTLE; // 通信同期処理などはここに追記

        for (int i = PVP; i <= TEAM_GREEN; i++) {
            if (isHoverIdx[i]) {
                SelectTeam(i);
                isBattlePlayer[0] = isTeamState[i];
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
}