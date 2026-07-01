// BaseLobbyScene.cpp
#include "BaseLobbyScene.h"
#include "Picture.h"
#include "DxLib.h"

extern Player g_player;

BaseLobbyScene::BaseLobbyScene(SelectScene::Option mode)
    : currentMode(mode)
{
    for (int i = 0; i < MAX; i++) { isHoverIdx[i] = false; }
}

void BaseLobbyScene::Draw() const {
    int str_but_x = 350;
    int str_but_y = 550;

    const char* firstLabel;
    switch (currentMode) {
    case SelectScene::Option::TRANING: firstLabel = _T("修行");     break;
    case SelectScene::Option::PVP:     firstLabel = _T("乱闘");     break;
    default:                           firstLabel = _T("真剣勝負"); break;
    }

    // 共通の背景と上下帯の描画
    DrawGraph(START_X, START_Y, Pic.GetSel(), TRUE);
    DrawBox(START_X, START_Y, WIN_MAX_X, 50, Col.GetSky(), TRUE);
    DrawBox(START_X, WIN_MAX_Y - 50, WIN_MAX_X, WIN_MAX_Y, Col.GetSky(), TRUE);

    // 共通の戻るボタン描画
    unsigned int color = isHoverIdx[RETURN] ? Col.GetCurYel() : Col.GetWhi();
    DrawBox(RET_BUT_X, RET_BUT_Y, RET_BUT_END_X, RET_BUT_END_Y, color, TRUE);
    DrawBox(RET_BUT_X, RET_BUT_Y, RET_BUT_END_X, RET_BUT_END_Y, Col.GetBla(), FALSE);
    DrawString(37, 17, _T("戻る"), Col.GetBla());

    // 共通のバトルスタートボタン描画
    // 後述する各子クラスの固有UI描画を呼び出す前に共通部分を処理
    if (ShouldDrawStartButton()) {
        if (isHoverIdx[BATTLE_START]) {
            Pic.MouseHoverDraw(str_but_x, str_but_y + 1, Pic.GetStartButton());
        }
        else {
            DrawGraph(str_but_x, str_but_y, Pic.GetStartButton(), TRUE);
        }
    }

    // 各モード固有のUIをここに挟み込む
    DrawSpecificUI();

    // 共通の下部ステータス表示
    DrawFormatStringToHandle(10, WIN_MAX_Y - 30, Col.GetBla(), Font.GetSmall(), _T("Name: %s"), g_player.getName().c_str());
    DrawStringToHandle(110, 10, firstLabel, Col.GetBla(), Font.GetNormal());
}

void BaseLobbyScene::DrawPlayerTeam(const std::string& nameStr, int y, unsigned int bgColor) const {
    int start_name_x = 300;
    int end_name_x = 700;
    DrawCircle(start_name_x, y, 15, Col.GetBla(), FALSE);
    DrawCircle(end_name_x, y, 15, Col.GetBla(), FALSE);
    DrawBox(start_name_x, y - 15, end_name_x, y + 16, Col.GetBla(), FALSE);

    DrawCircle(start_name_x, y, 14, bgColor, TRUE);
    DrawCircle(end_name_x, y, 14, bgColor, TRUE);
    DrawBox(start_name_x + 1, y - 14, end_name_x - 1, y + 15, bgColor, TRUE);

    const char* name = nameStr.c_str();
    int centerX = 500;
    int width = GetDrawStringWidth(name, (int)strlen(name));
    int drawX = centerX - (width / 2);
    int drawY = y - 8;
    DrawString(drawX, drawY, name, Col.GetBla());
}

void BaseLobbyScene::BlackDrawBox(int x, int y, int x2, int y2) const {
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
    DrawBox(x, y, x2, y2, Col.GetBla(), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}