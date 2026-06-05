// SettingScene.cpp
#include "SettingScene.h"
#include "InputManager.h"
#include "Player.h"
#include "Picture.h"
#include "GameConfig.h"

// プレイヤー情報のグローバル変数（以前のコードから推測）
extern Player g_player;

// ==========================================
// コンストラクタ＆初期化
// ==========================================
SettingScene::SettingScene(SelectScene::Option mode)
    : currentMode(mode)
    , selectedOption(NONE)
    , MemberCustom(false)
    , selectedMemberCount(2)
{
    // 配列の初期化
    for (int i = 0; i < MAX; i++) { isHoverIdx[i] = false; isTeam[i] = false; }
    for (int i = 0; i < 9; i++) { isHoverIdx2[i] = false; }
    for (int i = 0; i < MEMBER_MAX; i++) { isBattlePlayer[i] = false; }
}

// ヘルパー関数の実装
void SettingScene::DrawPlayerTeam(const std::string& nameStr, int y) const {
    // 名前表示用のUI描画
    int start_name_x = 300;
    int end_name_x = 700;
    DrawCircle(start_name_x, y, 15, Col.GetBla(), FALSE);
    DrawCircle(end_name_x, y, 15, Col.GetBla(), FALSE);
    DrawBox(start_name_x, y - 15, end_name_x, y + 16, Col.GetBla(), FALSE);
    DrawCircle(start_name_x, y, 14, Col.GetWhi(), TRUE);
    DrawCircle(end_name_x, y, 14, Col.GetWhi(), TRUE);
    DrawBox(start_name_x + 1, y - 14, end_name_x - 1, y + 15, Col.GetWhi(), TRUE);

    const char* name = nameStr.c_str();
    int centerX = 500;
    int width = GetDrawStringWidth(name, (int)strlen(name));
    int drawX = centerX - (width / 2);
    int drawY = y - 8;

    DrawString(drawX, drawY, name, Col.GetBla());
}

void SettingScene::BlackDrawBox(int x, int y, int x2, int y2) const {
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
    DrawBox(x, y, x2, y2, Col.GetBla(), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void SettingScene::SelectTeam(int teamId) {
    // PVP(個人)? TEAM_GREEN(緑チーム)までの選択状態を一旦すべてfalse(未所属)にする
    for (int i = PVP; i <= TEAM_GREEN; i++) {
        isTeam[i] = false;
    }

    // 押されたボタンの場所だけtrue(所属)にする
    if (teamId >= PVP && teamId <= TEAM_GREEN) {
        isTeam[teamId] = true;
    }
}

// 更新処理
SceneName SettingScene::Update(const InputManager& input) {
    int btnW = 200, btnH = 100, startX = 750;

    for (int i = 0; i < MAX; i++) { isHoverIdx[i] = false; }

    // --- ホバー判定 ---
    for (int i = 0; i < MAX; i++) {
        if (i == RETURN) {
            isHoverIdx[i] = input.IsMouseOver(10, 10, 100, 30);
        }
        else if (i == BATTLE_START) {
            if (!MemberCustom) { isHoverIdx[i] = input.IsMouseOver(350, 550, 300, 150); }
        }
        else {
            switch (currentMode) {
            case SelectScene::Option::TRANING:
                if (i == MEMBER) {
                    if (MemberCustom && input.IsLeftClicked()) {
                        if (!input.IsMouseOver(200, 150, 650, 375)) { MemberCustom = false; }
                    }
                    if (!MemberCustom) {
                        isHoverIdx[i] = input.IsMouseOver(200, 250, 600, 100);
                    }
                    // 人数選択ウィンドウ内の判定
                    if (MemberCustom) {
                        int j = 0, member_num = 2;
                        for (int y = 0; y < 3; y++) {
                            for (int x = 0; x < 3; x++) {
                                if (member_num > MEMBER_MAX) break;
                                int PosX = 250 + (x * 200), PosY = 200 + (y * 100);
                                isHoverIdx2[j] = input.IsMouseOver(PosX, PosY, 150, 80);

                                if (input.IsLeftClicked() && isHoverIdx2[j]) {
                                    selectedMemberCount = member_num;
                                    MemberCustom = false;
                                }
                                member_num++; j++;
                            }
                        }
                    }
                }
                break;
            case SelectScene::Option::PVP:
                if (i >= TEAM_RED && i <= TEAM_GREEN) {
                    int num = i - TEAM_RED;
                    int btnY = 100 + (num * 104);
                    isHoverIdx[i] = input.IsMouseOver(startX, btnY, btnW, btnH);
                }
                else if (i == PVP) {
                    isHoverIdx[i] = input.IsMouseOver(50, 100, btnW, btnH);
                }
                break;
            case SelectScene::Option::TAIMAN:
                if (i == RANKING) { isHoverIdx[i] = input.IsMouseOver(50, 100, btnW, btnH); }
                break;
            default: break;
            }
        }

        // --- クリック確定処理 ---
        if (input.IsLeftClicked() && isHoverIdx[i]) {
            selectedOption = i;
            switch (selectedOption) {
            case BATTLE_START:
                BattlePlayer.clear();
                {
                    Player user;
                    user.setName(g_player.getName());
                    user.setControllerType(ControllerType::HUMAN);
                    BattlePlayer.push_back(user);
                }
                for (int n = 1; n < selectedMemberCount; n++) {
                    Player ai;
                    std::string aiName = "AI " + std::to_string(n);
                    ai.setName(aiName.c_str());
                    ai.setControllerType(ControllerType::AI);
                    BattlePlayer.push_back(ai);
                }
                // バトルシーンへ遷移
                return SceneName::BATTLE;

            case MEMBER:
                MemberCustom = true;
                break;

            case PVP:
            case TEAM_RED:
            case TEAM_BLUE:
            case TEAM_YELLOW:
            case TEAM_GREEN:
                SelectTeam(selectedOption);
                break;

            case RETURN:
                // セレクトシーンへ戻る
                return SceneName::SELECT;

            default:
                MemberCustom = false;
                break;
            }
        }
    }

    // 何も遷移がなければ現在のシーン（SETTING）を維持
    return SceneName::SETTING;
}

// ==========================================
// 描画処理 (Draw)
// ==========================================
void SettingScene::Draw() const {
    // スタートボタンの原点座標
    int str_but_x = 350;
    int str_but_y = 550;

    // モード名表示の原点座標
    int label_x = 110;

    const char* firstLabel;
    switch (currentMode) {
    case SelectScene::Option::TRANING: firstLabel = _T("修行");     break;
    case SelectScene::Option::PVP:     firstLabel = _T("乱闘");     break;
    default:                           firstLabel = _T("真剣勝負"); break;
    }

    DrawGraph(START_X, START_Y, Pic.GetSel(), TRUE);

    // 上下線の描画
    DrawBox(START_X, START_Y, WIN_MAX_X, 50, Col.GetSky(), TRUE);
    DrawBox(START_X, WIN_MAX_Y - 50, WIN_MAX_X, WIN_MAX_Y, Col.GetSky(), TRUE);

    for (int i = 0; i < MAX; i++) {
        if (i == RETURN) {
            unsigned int color = isHoverIdx[i] ? Col.GetCurYel() : Col.GetWhi();
            DrawBox(10, 10, 100, 40, color, TRUE);
            DrawBox(9, 9, 101, 41, Col.GetBla(), FALSE);
            DrawString(37, 17, _T("戻る"), Col.GetBla());
        }
        else if (i == BATTLE_START) {
            if (isHoverIdx[i]) { Pic.MouseHoverDraw(str_but_x, str_but_y + 1, Pic.GetStartButton()); }
            else { DrawGraph(str_but_x, str_but_y, Pic.GetStartButton(), TRUE); }
        }
        else {
            switch (currentMode) {
            case SelectScene::Option::TRANING:
                if (i == MEMBER) {
                    if (isHoverIdx[i]) { Pic.MouseHoverDraw(200, 251, Pic.GetAIButton()); }
                    else { DrawGraph(200, 250, Pic.GetAIButton(), TRUE); }

                    DrawFormatStringToHandle(275, 265 + (isHoverIdx[i] ? 1 : 0), Col.GetBla(), Font.GetBig(), _T("対戦人数 : %d人"), selectedMemberCount);

                    if (MemberCustom) {
                        BlackDrawBox(0, 50, WIN_MAX_X, WIN_MAX_Y - 50);
                        DrawBox(200, 150, 850, 525, Col.GetWhi(), TRUE);

                        int member_num = 2, j = 0;
                        for (int y = 0; y < 3; y++) {
                            for (int x = 0; x < 3; x++) {
                                if (member_num > MEMBER_MAX) break;
                                int PosX = 250 + (x * 200), PosY = 200 + (y * 100);

                                if (isHoverIdx2[j]) { Pic.MouseHoverDraw(PosX, PosY + 1, Pic.GetMember()); }
                                else { DrawGraph(PosX, PosY, Pic.GetMember(), TRUE); }

                                DrawFormatStringToHandle(PosX + 30, PosY + 5 + (isHoverIdx2[j] ? 1 : 0), Col.GetBla(), Font.GetBig(), _T("%d人"), member_num);
                                member_num++; j++;
                            }
                        }
                    }
                }
                break;

            case SelectScene::Option::PVP:
                if (i >= TEAM_RED && i <= TEAM_GREEN) {
                    int num = i - TEAM_RED;
                    int btnY = 100 + (num * 104);
                    if (isHoverIdx[i]) { Pic.MouseHoverDraw(750, btnY + 1, Pic.GetTeamButton(num)); }
                    else { DrawGraph(750, btnY, Pic.GetTeamButton(num), TRUE); }
                }
                else if (i == PVP) {
                    if (isHoverIdx[i]) { Pic.MouseHoverDraw(50, 100, Pic.GetSoloButton()); }
                    else { DrawGraph(50, 100, Pic.GetSoloButton(), TRUE); }
                }

                for (int j = 0; j < MEMBER_MAX; j++) {
                    if (isBattlePlayer[j]) { DrawPlayerTeam(g_player.getName(), 100 + (j * 40)); }
                    else { DrawPlayerTeam(_T(" "), 100 + (j * 40)); }
                }
                break;
            default: break;
            }
        }
    }

    DrawFormatStringToHandle(10, WIN_MAX_Y - 30, Col.GetBla(), Font.GetSmall(), _T("Name: %s"), g_player.getName().c_str());
    DrawStringToHandle(110, 10, firstLabel, Col.GetBla(), Font.GetNormal());
}