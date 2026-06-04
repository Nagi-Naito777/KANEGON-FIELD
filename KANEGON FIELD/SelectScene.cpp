#include "SelectScene.h"
#include "DxLib.h"
#include "Picture.h"
#include "GameConfig.h"

// コンストラクタ
SelectScene::SelectScene() : selectedOption(NONE) {
    for (int i = 0; i < MAX; i++) {
        isHoverIdx[i] = false;
    }
}

// デストラクタ
SelectScene::~SelectScene() {
}

// --- 更新処理 ---
SceneName SelectScene::Update(const InputManager& input) {
    int btnW = 600;
    int btnH = 100;

    // 判定の初期化
    for (int i = 0; i < MAX; i++) {
        isHoverIdx[i] = false;
    }

    for (int i = 0; i < MAX; i++) {
        // マウスが乗っているかの判定（InputManagerにお任せ！）
        if (i == RETURN) {
            isHoverIdx[i] = input.IsMouseOver(10, 10, 100, 30);
        }
        else {
            isHoverIdx[i] = input.IsMouseOver(SELBUT_START_X, 100 + (i * 150), btnW, btnH);
        }

        // 左クリックされた瞬間の処理（InputManagerにお任せ！）
        if (input.IsLeftClicked() && isHoverIdx[i]) {
            selectedOption = i;

            // 選ばれたボタンに応じて、次に移動するシーンを返す
            switch (i) {
            case RETURN:
                return SceneName::TITLE;  // タイトルへ戻る

            case TRANING:
            case PVP:
            case TAIMAN:
                return SceneName::BATTLE; // バトルへ進む
            }
        }
    }

    // 何も選ばれていなければ、セレクト画面のまま
    return SceneName::SELECT;
}

// --- 描画処理 ---
void SelectScene::Draw() const {
    // 背景画像
    DrawGraph(START_X, START_Y, Pic.GetSel(), TRUE);

    // 上下のラインを描画
    DrawBox(START_X, START_Y, WIN_MAX_X, LINE_END_Y, GetColor(0, 255, 255), TRUE);
    DrawBox(START_X, LINE_START_Y, WIN_MAX_X, WIN_MAX_Y, GetColor(0, 255, 255), TRUE);

    // 入力した名前を表示 (g_player から取得)
    DrawFormatStringToHandle(
        // 戻るボタンのX座標と同じところから開始
        RET_BUT_X, NAME_START_Y,
        Col.GetBla(),
        Font.GetSmall(),
        _T("Name: %s"),
        g_player.getName().c_str()
    );

    // ボタンの描画
    for (int i = 0; i < MAX; i++) {
        int y = 100 + (i * 150);

        if (i == RETURN) {
            unsigned int color = isHoverIdx[i] ? Col.GetCurYel() : Col.GetWhi();
            DrawBox(RET_BUT_X, RET_BUT_Y, RET_BUT_END_X, RET_BUT_END_Y, color, TRUE);
            DrawBox(RET_BUT_X, RET_BUT_Y, RET_BUT_END_X, RET_BUT_END_Y, Col.GetBla(), FALSE);
            DrawString(37, 17, _T("戻る"), Col.GetBla());
        }
        else {
            // マウスカーソルが重なったときにグレーにする処理
            if (isHoverIdx[i]) {
                Pic.MouseHoverDraw(SELBUT_START_X, y + 1, Pic.GetSelButton(i));
            }
            else {
                DrawGraph(SELBUT_START_X, y, Pic.GetSelButton(i), TRUE);
            }
        }
    }
}