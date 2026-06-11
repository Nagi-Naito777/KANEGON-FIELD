#include "BattleUIManager.h"
#include "BattleData.h"
#include "GameConfig.h"
#include "Picture.h"
#include "DxLib.h"

extern Picture Pic;
extern FontManager Font;

void BattleUIManager::Draw(const BattleData& data) const {
    // プレイヤーがいない場合はエラー（アクセス違反）を防ぐために処理を中止する
    if (data.Player_Turn.empty()) {
        return;
    }

    //　背景画像の描画
    DrawGraph(START_X, START_Y, Pic.GetBat(), TRUE);

    // 上下のラインを描画
    DrawBox(START_X, START_Y, WIN_MAX_X, 50, Col.GetSky(), TRUE);
    DrawBox(START_X, WIN_MAX_Y - 50, WIN_MAX_X, WIN_MAX_Y, Col.GetSky(), TRUE);

    // 戻るボタンの描画
    unsigned int color = data.isHoverIdx[BattleOption::RETURN] ? Col.GetCurYel() : Col.GetWhi();
    DrawBox(RET_BUT_X, RET_BUT_Y, RET_BUT_END_X, RET_BUT_END_Y, color, TRUE);
    DrawBox(RET_BUT_X, RET_BUT_Y, RET_BUT_END_X, RET_BUT_END_Y, Col.GetBla(), FALSE);
    DrawString(37, 17, _T("戻る"), Col.GetBla());

    // プレイヤー一覧（ステータス）を描画
    DrawPlayerStatus(data, data.isHoverPlayerIdx);

    // ターンプレイヤー名とターゲット名
    DrawTurnPlayerName(data.Player_Turn[data.currentTurnIdx]);
    DrawTargetPlayerName(data);

    // 操作プレイヤー（人間）を探して情報を取得
    const Player* humanPlayer = nullptr;
    int humanIdx = -1;
    for (size_t i = 0; i < data.Player_Turn.size(); ++i) {
        if (data.Player_Turn[i].getControllerType() == ControllerType::HUMAN) {
            humanPlayer = &data.Player_Turn[i];
            humanIdx = (int)i;
            break;
        }
    }

    if (humanPlayer) {
        // 手札の描画
        DrawPlayerHand(data, *humanPlayer, data.hoveredCardIdx, data.isHoverCardIdx);

        // フェーズごとの選択UI描画
        if (data.currentPhase == BattlePhase::Select) {
            // 攻撃選択中のカード表示
            DrawSelectedCard(data, *humanPlayer, data.currentYOffset);
            DrawCardSelectButton(data.isHoverIdx);
        }
        else if (data.currentPhase == BattlePhase::DefenseSelect) {
            // 防御選択中のカード表示
            DrawDefenseCards(data, *humanPlayer, humanIdx, data.currentYOffset);
            DrawCardSelectButton(data.isHoverIdx);
        }
    }

    // 降参確認ウィンドウ（もしあれば）
    if (data.isSurrenderConfirm) {
        DrawSurrenderWindow(data);
    }
}

// プレイヤーステータスの描画
void BattleUIManager::DrawPlayerStatus(const BattleData& data, const bool* isHoverPlayerIdx) const {
    const int startX = 700;         // X開始点
    const int startY = 75;          // 1人目のY開始点
    const int marginY = 40;         // プレイヤーごとのUIの間隔

    for (size_t i = 0; i < data.Player_Turn.size(); i++) {
        int currentY = startY + (int)i * marginY;

        // --- ホバー中またはターゲット選択中なら色を変える ---
        unsigned int bgColor = GetColor(255, 255, 255); // 基本は白
        if (isHoverPlayerIdx[i]) {
            bgColor = Col.GetCurYel();  // 薄黄色
        }
        if (data.playerTarget && data.targetIdx == i) {
            bgColor = Col.GetRed(); // 選択済みのターゲットは赤色
        }

        // 名前UIの枠の描画
        const int Cir_r = 15;   // 半径変数

        DrawCircle(startX, currentY, Cir_r, Col.GetBla(), FALSE);
        DrawCircle(startX + 275, currentY, Cir_r, Col.GetBla(), FALSE);
        DrawBox(startX, currentY - 15, startX + 275, currentY + 16, Col.GetBla(), FALSE);

        // 名前UIの背景色の描画
        DrawCircle(startX, currentY, Cir_r - 1, bgColor, TRUE);
        DrawCircle(startX + 275, currentY, Cir_r - 1, bgColor, TRUE);
        DrawBox(startX, currentY - 14, startX + 275, currentY + 15, bgColor, TRUE);

        // 名前表示
        DrawFormatStringToHandle(
            startX, currentY - 7,
            GetColor(0, 155, 155),
            Font.GetSmall(),
            _T("%s"),
            data.Player_Turn[i].getName().c_str()
        );

        // プレイヤーのステータス表示
        DrawFormatStringToHandle(
            startX + 135, currentY - 7,
            Col.GetBla(),
            Font.GetSmall(),
            _T("HP %2d MP %2d ￥ %2d "),
            data.Player_Turn[i].getHp(), data.Player_Turn[i].getMp(), data.Player_Turn[i].getMoney()
        );
    }
}

// 手札の描画
void BattleUIManager::DrawPlayerHand(const BattleData& data, const Player& player,
    int hoveredCardIdx, const bool* isHoverCardIdx) const {
    // 手札を取得
    const auto& hand = player.Hand.GetCards();

    // デバッグ：画面左上に手札枚数を表示
    // DrawFormatString(0, 100, GetColor(0, 0, 0), "HandSize: %d", hand.size());

    // --- サイズ・レイアウト設定 ---
    const float SCALE = 1.45f;                  // 拡大倍率
    const int BASE_W = 50;                      // 元のカード幅
    const int BASE_H = 50;                      // 元のカード高さ
    const int CARD_W = (int)(BASE_W * SCALE);   // 拡大後の幅 (100)
    const int CARD_H = (int)(BASE_H * SCALE);   // 拡大後の高さ (100)

    const int Start_X = 10;                     // 1枚目のX座標
    const int Start_Y = 450;                    // 手札を表示するY座標（サイズアップに合わせて少し上に調整）
    const int MARGIN = 2;                       // カード同士の隙間（2倍に調整）

    // 改行用の変数
    const int MAX_CARDS_PER_ROW = 9;            // 1段の枚数
    const int ROW_SPACING = CARD_H + 30;        // 段ごとの縦の間隔

    // 攻撃ターンかどうか判定
    bool isAttackTurn = (player.getName() ==
        data.Player_Turn[data.currentTurnIdx].getName());

    // カード本体の描画ループ
    for (int i = 0; i < hand.size(); i++) {
        int col = i % MAX_CARDS_PER_ROW;
        int row = i / MAX_CARDS_PER_ROW;

        int x = Start_X + (CARD_W + MARGIN) * col;
        int y = Start_Y + (ROW_SPACING * row);

        // このカードが今のターンで使えるか判定
        bool isSelectable = false;
        int cat = hand[i].GetCategory();

        // 攻防カードは効果が異なるため分岐
        if (isAttackTurn) {
            if (cat != Defense) isSelectable = true;
        }
        else {
            if (cat == Defense || cat == Bilingual) isSelectable = true;
        }

        // カード画像の描画
        int picIdx = hand[i].graphicIndex;

        if (picIdx >= 0 && picIdx < 100) {
            // もしマウスカーソルが重なった時は若干白くさせる
            if (isHoverCardIdx[i]) {
                // ブレンドモードを「加算」に設定（0?255で白さの強さを調節）
                SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
            }
            // DrawExtendGraph(左上X, 左上Y, 右下X, 右下Y, グラフィックハンドル, 透過フラグ)
            DrawExtendGraph(x, y, x + CARD_W, y + CARD_H, Pic.GetCard(picIdx), TRUE);

            // 描き終わったら必ず「ノーブレンド」に戻す
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
        else {
            // エラー時の赤い箱も拡大サイズに合わせる
            DrawBox(x, y, x + CARD_W, y + CARD_H, Col.GetRed(), TRUE);
            printfDx(_T("Error: CardIndex %d out of range!\n"), picIdx);
        }

        // カードの枠線
        DrawBox(x, y, x + CARD_W, y + CARD_H, Col.GetBla(), FALSE);

        // 属性効果のあるカードのフォントカラーを変更する分岐
        int Ele_Col = Col.GetBla();
        if (hand[i].GetType() == "炎") { Ele_Col = GetColor(255, 0, 0); }
        else if (hand[i].GetType() == "水") { Ele_Col = GetColor(0, 0, 255); }
        else if (hand[i].GetType() == "木") { Ele_Col = GetColor(0, 155, 0); }
        else if (hand[i].GetType() == "光") { Ele_Col = GetColor(155, 155, 0); }
        else if (hand[i].GetType() == "闇") { Ele_Col = GetColor(255, 100, 255); }

        // テキストエリアの設定（カードのすぐ下に配置）
        int textAreaY = y + CARD_H;
        int textAreaH = 25; // テキスト背景の高さ
        DrawBox(x, textAreaY, x + CARD_W, textAreaY + textAreaH, Col.GetBoxYel(), TRUE);

        // カテゴリ別テキスト描画
        TCHAR buf[64];
        switch (hand[i].GetCategory()) {
        case Attack:
            _stprintf_s(buf, hand[i].GetAdd() ? _T("+攻%d") : _T("攻%d"), hand[i].GetPower());
            break;
        case Bilingual:
            if (data.currentPhase == BattlePhase::Select)
                _stprintf_s(buf, isAttackTurn ? _T("攻%d") : _T("守%d"), hand[i].GetPower());
            else
                _stprintf_s(buf, _T("守%d"), hand[i].GetPower());
            break;
        case Magic:
            if (hand[i].GetPower() > 0)
                _stprintf_s(buf, hand[i].GetAdd() ? _T("+攻%d") : _T("攻%d"), hand[i].GetPower());
            break;
        case Defense:
            _stprintf_s(buf, _T("守%d"), hand[i].GetPower());
            break;
        case All:
            _stprintf_s(buf, _T("%d%%攻%d"), hand[i].GetPercent(), hand[i].GetPower());
            break;
        case Buy:break;
        case Sell:break;
        case Change:break;
        }

        int w = GetDrawStringWidth(buf, (int)_tcslen(buf));
        DrawString(x + (CARD_W - w) / 2, textAreaY + 4, buf, Ele_Col);

        // 選択不可の暗転処理
        if ((data.currentPhase == BattlePhase::Select || data.currentPhase == BattlePhase::DefenseSelect) && !isSelectable) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
            DrawBox(x, y, x + CARD_W, y + CARD_H + textAreaH, Col.GetBla(), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }

    // マウスカーソルが重なった際に説明文を表示する処理
    if (hoveredCardIdx != -1 && hoveredCardIdx < hand.size()) {
        const auto& card = hand[hoveredCardIdx];

        // 選択してるカードの座標を再計算

        // レイアウト定数
        const int BOX_X1 = 685; // 説明ボックスのX開始点
        const int BOX_Y1 = 450; // Y開始点
        const int BOX_X2 = 995; // X終了点
        const int BOX_Y2 = 600; // Y終了点
        const int PADDING = 10; // ボックス内の余白

        // 背景ボックスの描画
        DrawBox(BOX_X1, BOX_Y1, BOX_X2, BOX_Y2, Col.GetBoxYel(), TRUE);      // 背景
        DrawBox(BOX_X1, BOX_Y1, BOX_X2, BOX_Y2, Col.GetWhi(), FALSE);           // 枠線

        // カード画像の描画変数
        const float img_s = 1.5f;               // 画像拡大率
        const int img_w = (int)(50 * img_s);    // 横幅
        const int img_h = (int)(50 * img_s);    // 縦幅

        // 画像を配置変数
        int imgX = BOX_X1 + PADDING;
        int imgY = BOX_Y1 + PADDING + 25;       // 名前表示の分だけ下げる

        // 画像描画
        DrawExtendGraph(imgX, imgY, imgX + img_w, imgY + img_h, Pic.GetCard(card.graphicIndex), TRUE);
        DrawBox(imgX, imgY, imgX + img_w, imgY + img_h, Col.GetBla(), FALSE);

        // カード名テキスト
        int card_txt_x = 710;
        int card_txt_y = 460;
        DrawFormatString(card_txt_x, card_txt_y, Col.GetBla(), _T("[%s]"), card.GetName().c_str());

        // --- 4. 説明文の描画 (画像の右側に改行して表示) ---
        int textX = imgX + img_w + PADDING;
        int textY = imgY;

        DrawFormatString(textX, textY, Col.GetBla(), _T("%s"), card.GetDescription().c_str());

        // 金額表示(奇跡のみ表示しない)
        if (card.GetCategory() != Magic) {
            DrawFormatString(textX, textY + 40, Col.GetBla(), _T("\\%d"), card.GetMoney());
        }
        if (card.GetCategory() == Bilingual) {
            DrawFormatString(textX, textY + 20, Col.GetBla(), _T("攻%d 守%d"), card.GetPower(), card.GetPower());
        }
        if (card.GetCategory() == Healing) {
            DrawFormatString(textX, textY + 20, Col.GetBla(), _T("HP+%d"), card.GetPower());
        }
        if (card.GetCategory() == MagicHealing) {
            DrawFormatString(textX, textY + 20, Col.GetBla(), _T("MP+%d"), card.GetPower());
        }

        // 奇跡の消費MPを表示
        if (card.GetCategory() == Magic) {
            DrawFormatString(textX, textY + 20, GetColor(50, 50, 255), _T("MP-%d"), card.GetMP());
        }

    }
}

// 選択されたカードを描画する関数
void BattleUIManager::DrawSelectedCard(const BattleData& data, const Player& player,
    float currentYOffset)const {
    // まだカードが選ばれていない、または手札の範囲外なら何もしない
    if (data.selectedCards.empty())return;

    const auto& hand = player.Hand.GetCards();  // 手札参照

    // サイズ設定（手札より少し小さめ）
    const float SCALE = 1.0f;
    const int CARD_W = (int)(50 * SCALE);
    const int CARD_H = (int)(50 * SCALE);

    // 描画開始座標
    int startX = 15;
    int startY = 95;

    // UIボックスの固定サイズ
    const int boxWidth = 250;

    // アニメーション変数(Updateで計算されたものを適用)
    int yOffset = (int)currentYOffset;

    // --- 選択されたすべてのカードを縦リストとして描画 ---
    for (int i = 0; i < (int)data.selectedCards.size(); ++i) {
        int handIdx = data.selectedCards[i];
        if (handIdx < 0 || handIdx >= (int)hand.size()) continue;

        const auto& card = hand[handIdx];

        int drawX = startX;
        int drawY = startY + (i * yOffset); // アニメーションするyOffsetで配置

        // 背面のテキストエリア（UIボックス）の描画
        DrawBox(drawX - 5, drawY - 5, drawX + boxWidth, drawY + CARD_H + 5, Col.GetBoxYel(), TRUE);
        DrawBox(drawX - 5, drawY - 5, drawX + boxWidth, drawY + CARD_H + 5, Col.GetBla(), FALSE);

        // カード画像の描画
        int picIdx = card.graphicIndex;
        if (picIdx >= 0 && picIdx < 100) {
            DrawExtendGraph(drawX, drawY, drawX + CARD_W, drawY + CARD_H, Pic.GetCard(picIdx), TRUE);
        }
        DrawBox(drawX, drawY, drawX + CARD_W, drawY + CARD_H, Col.GetBla(), FALSE);

        int textX = drawX + CARD_W + 15;

        // 属性色の取得
        int Ele_Col = Col.GetBla();
        if (card.GetType() == "炎") { Ele_Col = GetColor(255, 0, 0); }
        else if (card.GetType() == "水") { Ele_Col = GetColor(0, 0, 255); }
        else if (card.GetType() == "木") { Ele_Col = GetColor(0, 155, 0); }
        else if (card.GetType() == "光") { Ele_Col = GetColor(155, 155, 0); }
        else if (card.GetType() == "闇") { Ele_Col = GetColor(255, 100, 255); }

        // カテゴリ別文字描画
        TCHAR buf[64] = _T("");
        bool hasText = true;
        switch (card.GetCategory()) {
        case Attack:
        case Bilingual:
            _stprintf_s(buf, _T("攻%d"), card.GetPower());
            break;
        case Magic:
            if (card.GetPower() > 0)
                _stprintf_s(buf, card.GetAdd() ? _T("+攻%d") : _T("攻%d"), card.GetPower());
            else hasText = false;
            break;
        case Defense:
            _stprintf_s(buf, _T("守%d"), card.GetPower());
            break;
        case All:
            _stprintf_s(buf, _T("%d%%攻%d"), card.GetPercent(), card.GetPower());
            break;
        case Healing:
            _stprintf_s(buf, _T("HP+%d"), card.GetPower());
            break;
        case MagicHealing:
            _stprintf_s(buf, _T("MP+%d"), card.GetPower());
            break;
        default:
            hasText = false;
            break;
        }

        if (hasText) {
            int textX = drawX + CARD_W + 15;
            DrawFormatString(textX, drawY + 2, Ele_Col, _T("[%s]"), card.GetName().c_str());
            DrawString(textX, drawY + 22, buf, Ele_Col);
        }

    }

    // 合計威力の表示（手札一覧の少し上に表示。ここも連動して滑らかに動きます） 
    int baseIdx = data.selectedCards[0];
    bool isBaseHealCard = false;
    if (baseIdx >= 0 && baseIdx < (int)hand.size()) {
        CardCategory baseCat = hand[baseIdx].GetCategory();
        if (baseCat == Healing || baseCat == MagicHealing) {
            isBaseHealCard = true;
        }
    }

    if (!isBaseHealCard) {
        bool isAttackTurn = (player.getName() == data.Player_Turn[data.currentTurnIdx].getName());

        int totalDrawX = startX + 100;
        int totalDrawY = 400; // ※手札の少し上の位置に固定

        // 背景枠描画
        DrawBox(startX, 395, startX + 271, 395 + 25, Col.GetWhi(), TRUE);
        DrawBox(startX, 395, startX + 271, 395 + 25, Col.GetBla(), FALSE);

        // 属性・ターンに応じた色決定
        int totalCol = Col.GetBla();
        if (isAttackTurn) {
            if (data.currentAttackElement == "炎") totalCol = GetColor(255, 0, 0);
            else if (data.currentAttackElement == "水") totalCol = GetColor(0, 0, 255);
            else if (data.currentAttackElement == "木") totalCol = GetColor(0, 155, 0);
            else if (data.currentAttackElement == "光") totalCol = GetColor(155, 155, 0);
            else if (data.currentAttackElement == "闇") totalCol = GetColor(255, 100, 255);

            DrawFormatStringToHandle(totalDrawX, totalDrawY, totalCol, Font.GetSmall(), _T("攻 %d"), data.totalPower);
        }
        else {
            DrawFormatStringToHandle(totalDrawX, totalDrawY, GetColor(0, 255, 255), Font.GetSmall(), _T("守 %d"), data.totalPower);
        }
    }
}

// BattleInput がクリック判定に使うための関数
Rect BattleUIManager::GetHandCardRect(int handIndex) const {
    static constexpr float CARD_SCALE = 1.45f;
    static constexpr int BASE_CARD_W = 50;
    static constexpr int BASE_CARD_H = 50;
    static constexpr int HAND_START_X = 10;
    static constexpr int HAND_START_Y = 450;
    static constexpr int MAX_CARDS_PER_ROW = 9;
    static constexpr int CARD_MARGIN = 2;
    static constexpr int ROW_SPACING = (int)(BASE_CARD_H * CARD_SCALE) + 30;


    int col = handIndex % MAX_CARDS_PER_ROW;
    int row = handIndex / MAX_CARDS_PER_ROW;

    int cardW = (int)(BASE_CARD_W * CARD_SCALE);
    int cardH = (int)(BASE_CARD_H * CARD_SCALE);

    int x = HAND_START_X + (cardW + CARD_MARGIN) * col;
    int y = HAND_START_Y + (ROW_SPACING * row);

    // 新しいRect構造体で返す
    return { x, y, cardW, cardH };
}

// 今のターンのプレイヤー名表示
void BattleUIManager::DrawTurnPlayerName(const Player& player)const {
    int x = 15;
    int y = 70;
    int r = 10;
    int boxWidth = 250;

    int stringWidth = GetDrawStringWidthToHandle(
        player.getName().c_str(),
        (int)_tcslen(player.getName().c_str()),
        Font.GetSmall()
    );
    int drawX = x + (boxWidth - stringWidth) / 2;

    DrawCircle(x, y, r, Col.GetBla(), FALSE);
    DrawCircle(x + boxWidth, y, r, Col.GetBla(), FALSE);
    DrawBox(x, y - 10, x + boxWidth, y + 11, Col.GetBla(), FALSE);

    DrawCircle(x, y, r - 1, Col.GetWhi(), TRUE);
    DrawCircle(x + boxWidth, y, r - 1, Col.GetWhi(), TRUE);
    DrawBox(x, y - 9, x + boxWidth, y + 10, Col.GetWhi(), TRUE);

    DrawFormatStringToHandle(drawX, y - 7, GetColor(200, 50, 50), Font.GetSmall(), _T("%s"), player.getName().c_str());
}

// ターゲット指定されたプレイヤー名表示
void BattleUIManager::DrawTargetPlayerName(const BattleData& data) const {
    if (data.targetIdx < 0 || data.targetIdx >= (int)data.Player_Turn.size()) return;

    int x = 350;
    int y = 70;
    int r = 10;
    int boxWidth = 250;
    int arrowColor = GetColor(0, 0, 0);
    int arrowY = y;

    if (data.targetIdx != data.currentTurnIdx) {
        int stringWidth = GetDrawStringWidthToHandle(
            data.Player_Turn[data.targetIdx].getName().c_str(),
            (int)_tcslen(data.Player_Turn[data.targetIdx].getName().c_str()),
            Font.GetSmall()
        );
        int drawX = x + (boxWidth - stringWidth) / 2;

        DrawCircle(x, y, r, Col.GetBla(), FALSE);
        DrawCircle(x + boxWidth, y, r, Col.GetBla(), FALSE);
        DrawBox(x, y - 10, x + boxWidth, y + 11, Col.GetBla(), FALSE);

        DrawCircle(x, y, r - 1, Col.GetWhi(), TRUE);
        DrawCircle(x + boxWidth, y, r - 1, Col.GetWhi(), TRUE);
        DrawBox(x, y - 9, x + boxWidth, y + 10, Col.GetWhi(), TRUE);

        DrawFormatStringToHandle(drawX, y - 7, GetColor(200, 50, 50), Font.GetSmall(), _T("%s"), data.Player_Turn[data.targetIdx].getName().c_str());
    }

    if (data.targetIdx != data.currentTurnIdx) {
        int startX = 295, endX = 320;
        DrawLine(startX, arrowY, endX, arrowY, arrowColor, 2);
        DrawTriangle(endX, arrowY, endX - 10, arrowY - 5, endX - 10, arrowY + 5, arrowColor, TRUE);
    }
    else {
        int startX = 320, endX = 295;
        DrawLine(startX, arrowY, endX, arrowY, arrowColor, 2);
        DrawTriangle(endX, arrowY, endX + 10, arrowY - 5, endX + 10, arrowY + 5, arrowColor, TRUE);
    }
}

// 防御側のカード表示
void BattleUIManager::DrawDefenseCards(const BattleData& data,
    const Player& player, int humanIdx, float currentYOffset) const {


    // 防御フェーズかつ、自身がターゲットにされている場合のみ描画
    if (data.currentPhase == BattlePhase::DefenseSelect && data.targetIdx == humanIdx) {

        // まだカードが選ばれていない場合は何もしない
        if (data.selectedDefenseCards.empty()) return;

        const auto& hand = player.Hand.GetCards(); // 手札を参照

        // --- サイズ設定（手札より少し小さめ） ---
        const float SCALE = 1.0f;
        const int CARD_W = (int)(50 * SCALE);
        const int CARD_H = (int)(50 * SCALE);

        // --- 描画開始座標 ---
        // 攻撃側(startX=15)と対になるよう、ターゲット名(x=350)の位置に合わせる
        int startX = 350;
        int startY = 95;
        const int boxWidth = 250;

        // -------------------------------------------------------------
        // ★攻撃側（DrawSelectedCard）と完全に同じ変数をそのまま使う
        // -------------------------------------------------------------
        int yOffset = (int)currentYOffset;
        // -------------------------------------------------------------

        // --- 選択されたすべてのカードを縦リストとして描画 ---
        for (int i = 0; i < (int)data.selectedDefenseCards.size(); ++i) {
            int handIdx = data.selectedDefenseCards[i];
            if (handIdx >= 0 && handIdx < (int)hand.size()) {
                const auto& card = hand[handIdx];

                int drawX = startX;
                // 攻撃側と全く同じアニメーション計算
                int drawY = startY + (i * yOffset);

                // 1. 背面のテキストエリア（UIボックス）の描画
                DrawBox(drawX - 5, drawY - 5, drawX + boxWidth, drawY + CARD_H + 5, Col.GetBoxYel(), TRUE);
                DrawBox(drawX - 5, drawY - 5, drawX + boxWidth, drawY + CARD_H + 5, Col.GetBla(), FALSE);

                // 2. カード画像の描画
                int picIdx = card.graphicIndex;
                if (picIdx >= 0 && picIdx < 100) {
                    DrawExtendGraph(drawX, drawY, drawX + CARD_W, drawY + CARD_H, Pic.GetCard(picIdx), TRUE);
                }
                DrawBox(drawX, drawY, drawX + CARD_W, drawY + CARD_H, Col.GetBla(), FALSE);

                int textX = drawX + CARD_W + 15;

                // 3. 属性色の取得
                int Ele_Col = Col.GetBla();
                if (card.GetType() == "炎") { Ele_Col = GetColor(255, 0, 0); }
                else if (card.GetType() == "水") { Ele_Col = GetColor(0, 0, 255); }
                else if (card.GetType() == "木") { Ele_Col = GetColor(0, 155, 0); }
                else if (card.GetType() == "光") { Ele_Col = GetColor(155, 155, 0); }
                else if (card.GetType() == "闇") { Ele_Col = GetColor(255, 100, 255); }

                // 4. カテゴリごとの文字描画
                TCHAR buf[64] = _T("");
                bool hasText = true;

                switch (card.GetCategory()) {
                case Attack:
                case Bilingual:
                    _stprintf_s(buf, card.GetAdd() ? _T("守%d") : _T("守%d"), card.GetPower());
                    break;
                case Magic:
                    break;
                case Defense:
                    _stprintf_s(buf, _T("守%d"), card.GetPower());
                    break;
                default:
                    hasText = false;
                    break;
                }

                if (hasText) {
                    DrawFormatString(textX, drawY + 2, Ele_Col, _T("[%s]"), card.GetName().c_str());
                    DrawString(textX, drawY + 22, buf, Ele_Col);
                }
            }
        }

        // =============================================================
        // 合計防御力の表示（攻撃側と同じ高さに合わせて配置）
        // =============================================================
        int totalDrawX = startX + 100;
        int totalDrawY = 400; // 手札の少し上の位置

        // 合計値を見やすくするための枠組みの座標用変数
        const int totalbox_x = 340;
        const int totalbox_y = 395;
        DrawBox(totalbox_x, totalbox_y, totalbox_x + 271, totalbox_y + 25, Col.GetWhi(), TRUE);
        DrawBox(totalbox_x, totalbox_y, totalbox_x + 271, totalbox_y + 25, Col.GetBla(), FALSE);

        int totalDefCol = Col.GetBla(); // デフォルト色
        if (data.currentDefenseElement == "炎") { totalDefCol = GetColor(255, 0, 0); }
        else if (data.currentDefenseElement == "水") { totalDefCol = GetColor(0, 0, 255); }
        else if (data.currentDefenseElement == "木") { totalDefCol = GetColor(0, 155, 0); }
        else if (data.currentDefenseElement == "光") { totalDefCol = GetColor(155, 155, 0); }
        else if (data.currentDefenseElement == "闇") { totalDefCol = GetColor(255, 100, 255); }

        // DrawFormatString ではなく DrawFormatStringToHandle を使う
        DrawFormatStringToHandle(totalDrawX, totalDrawY, totalDefCol, Font.GetSmall(), _T("守 %d"), data.totalPower);
    }
}

void BattleUIManager::DrawSurrenderWindow(const BattleData& data) const {

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
    DrawBox(0, 50, 1000, 750, GetColor(0, 0, 0), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    DrawBox(300, 200, 700, 400, GetColor(255, 255, 255), TRUE);
    DrawBox(300, 200, 700, 400, GetColor(0, 0, 0), FALSE);

    DrawString(415, 240, "本当に降参しますか？", GetColor(0, 0, 0));

    unsigned int btnColor = data.isHoverIdx[(int)BattleOption::GIVE_UP] ? GetColor(255, 100, 100) : GetColor(200, 0, 0);
    DrawBox(425, 300, 575, 350, btnColor, TRUE);
    DrawString(460, 315, "あきらめる", GetColor(255, 255, 255));
}

// カード選択決定ボタンの描画
void BattleUIManager::DrawCardSelectButton(const bool* ishoverIdx)const {
    const int DECISION_AREA_W = 271;
    const int DECISION_AREA_H = 325;
    const int ATK_BTN_X = 5;
    const int ATK_BTN_Y = 60;
    const int DEF_BTN_X = 340;
    const int DEF_BTN_Y = ATK_BTN_Y;
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
    if (ishoverIdx[BattleOption::ATTACK]) {
        DrawBox(ATK_BTN_X, ATK_BTN_Y, ATK_BTN_X + DECISION_AREA_W, ATK_BTN_Y + DECISION_AREA_H, Col.GetWhi(), TRUE);
    }
    else if (ishoverIdx[BattleOption::DEFENSE]) {
        DrawBox(DEF_BTN_X, DEF_BTN_Y, DEF_BTN_X + DECISION_AREA_W, DEF_BTN_Y + DECISION_AREA_H, Col.GetWhi(), TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}