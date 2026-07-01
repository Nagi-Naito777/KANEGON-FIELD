#include "TrainingLobbyScene.h"
#include "Picture.h"
#include "DxLib.h"

extern Player g_player;
static int s_savedMemberCount = 2; // 前回の人数を記憶

TrainingLobbyScene::TrainingLobbyScene()
    : BaseLobbyScene(SelectScene::Option::TRANING)
    , MemberCustom(false)
    , selectedMemberCount(s_savedMemberCount)
{
    for (int i = 0; i < 9; i++) { isHoverIdx2[i] = false; }
}

SceneName TrainingLobbyScene::Update(const InputManager& input) {
    for (int i = 0; i < MAX; i++) { isHoverIdx[i] = false; }

    // 戻るボタン判定
    isHoverIdx[RETURN] = input.IsMouseOver(RET_BUT_X, RET_BUT_Y, RET_BUTW, RET_BUTH);

    // スタートボタン判定
    if (!MemberCustom) { isHoverIdx[BATTLE_START] = input.IsMouseOver(350, 550, 300, 150); }

    // 人数変更ボタン・ウィンドウの処理
    if (MemberCustom && input.IsLeftClicked()) {
        if (!input.IsMouseOver(200, 150, 650, 375)) { MemberCustom = false; }
    }
    if (!MemberCustom) {
        isHoverIdx[MEMBER] = input.IsMouseOver(200, 250, 600, 100);
    }

    if (MemberCustom) {
        int j = 0, member_num = 2;
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                if (member_num > MEMBER_MAX) break;
                int PosX = 250 + (x * 200), PosY = 200 + (y * 100);
                isHoverIdx2[j] = input.IsMouseOver(PosX, PosY, 150, 80);

                if (input.IsLeftClicked() && isHoverIdx2[j]) {
                    selectedMemberCount = member_num;
                    s_savedMemberCount = member_num;
                    MemberCustom = false;
                }
                member_num++; j++;
            }
        }
    }

    // クリック実行処理
    if (input.IsLeftClicked()) {
        if (isHoverIdx[RETURN]) return OnReturnClicked();
        if (isHoverIdx[MEMBER]) MemberCustom = true;
        if (isHoverIdx[BATTLE_START]) {
            BattlePlayer.clear();
            // 自分を追加
            Player user;
            user.setName(g_player.getName());
            user.setControllerType(ControllerType::HUMAN);
            BattlePlayer.push_back(user);

            // AIを追加
            for (int n = 1; n < selectedMemberCount; n++) {
                Player ai;
                std::string aiName = "AI " + std::to_string(n);
                ai.setName(aiName.c_str());
                ai.setControllerType(ControllerType::AI);
                BattlePlayer.push_back(ai);
            }
            return SceneName::BATTLE;
        }
    }
    return SceneName::SETTING;
}

void TrainingLobbyScene::DrawSpecificUI() const {
    int men_butX = 200, men_butY = 250;

    // 人数変更ボタンの描画
    if (isHoverIdx[MEMBER]) { Pic.MouseHoverDraw(men_butX, men_butY + 1, Pic.GetAIButton()); }
    else { DrawGraph(men_butX, men_butY, Pic.GetAIButton(), TRUE); }

    DrawFormatStringToHandle(275, 267 + (isHoverIdx[MEMBER] ? 1 : 0), Col.GetBla(), Font.GetBig(), _T("対戦人数 : %d人"), selectedMemberCount);

    // カスタムウィンドウ（ポップアップ）の描画
    if (MemberCustom) {
        int box_x = 200, box_y = 150, box_ex = 850, box_ey = 525;
        BlackDrawBox(START_X, START_Y + 50, WIN_MAX_X, WIN_MAX_Y - 50);
        DrawBox(box_x, box_y, box_ex, box_ey, Col.GetWhi(), TRUE);

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