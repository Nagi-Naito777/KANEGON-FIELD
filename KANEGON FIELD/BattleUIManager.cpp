#include "BattleUIManager.h"
#include "BattleData.h"
#include "GameConfig.h"
#include "Picture.h"
#include "DxLib.h"

extern Picture Pic;
extern FontManager Font;

// 属性の色処理を関数化
unsigned int BattleUIManager::GetElementColor(const std::string& elementType) const {
    if (elementType == "炎") return GetColor(255, 0, 0);
    if (elementType == "水") return GetColor(0, 0, 255);
    if (elementType == "木") return GetColor(0, 155, 0);
    if (elementType == "光") return GetColor(155, 155, 0);
    if (elementType == "闇") return GetColor(255, 100, 255);
    return Col.GetBla(); // デフォルト色
}

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

    // バトル終了フェーズの場合は、リザルト画面を描画して以降の処理をスキップ
    if (data.currentPhase == BattlePhase::End) {
        DrawEndScreen(data);
        return;
    }

    // 戻るボタンの描画
    unsigned int color = data.isHoverIdx[BattleOption::RETURN] ? Col.GetCurYel() : Col.GetWhi();
    DrawBox(RET_BUT_X, RET_BUT_Y, RET_BUT_END_X, RET_BUT_END_Y, color, TRUE);
    DrawBox(RET_BUT_X, RET_BUT_Y, RET_BUT_END_X, RET_BUT_END_Y, Col.GetBla(), FALSE);
    DrawString(37, 17, _T("戻る"), Col.GetBla());

    // プレイヤー一覧（ステータス）を描画
    DrawPlayerStatus(data, data.isHoverPlayerIdx);

    // ターンプレイヤー名とターゲット名
    const Player& attacker = data.Player_Turn[data.currentTurnIdx];
    DrawTurnPlayerName(attacker); // 攻撃側の名前は常に表示

    // ターゲット選択中(Select)または防衛・ダメージフェーズで矢印と名前を表示
    if ((data.currentPhase == BattlePhase::Select && data.playerTarget) ||
        data.currentPhase == BattlePhase::DefenseSelect ||
        data.currentPhase == BattlePhase::DamageResult) {
        DrawTargetPlayerName(data);
    }

    // --- カウンター（跳ね返し）待機中のUI表示 ---
    if (data.isPendingAttack) {
        int counterEleCol = GetElementColor(data.pendingAttackType);
        // ターゲット名表示の下あたりに反射ダメージの予告を描画
        DrawBox(350, 95, 600, 120, Col.GetWhi(), TRUE);
        DrawBox(350, 95, 600, 120, Col.GetBla(), FALSE);
        DrawFormatStringToHandle(355, 100, counterEleCol, Font.GetSmall(), _T("【反射待機】威力: %d"), data.pendingAttackPower);
    }

    // 攻撃カードの描画 (セレクト開始?ダメージ計算完了までずっと表示)
    if (data.currentPhase == BattlePhase::Select ||
        data.currentPhase == BattlePhase::DefenseSelect ||
        data.currentPhase == BattlePhase::Effect ||
        data.currentPhase == BattlePhase::DamageResult) {

        // 攻撃側プレイヤーの手札データを渡す
        DrawSelectedCard(data, attacker, data.currentYOffset);
    }

    // 防御カードの描画 (防衛セレクト開始?ダメージ計算完了までずっと表示)
    if (data.currentPhase == BattlePhase::DefenseSelect ||
        data.currentPhase == BattlePhase::Effect ||
        data.currentPhase == BattlePhase::DamageResult) {

        if (data.targetIdx >= 0 && data.targetIdx < data.Player_Turn.size()) {
            const Player& defender = data.Player_Turn[data.targetIdx];
            // 防衛側プレイヤーの手札データを渡す
            DrawDefenseCards(data, defender, data.targetIdx, data.currentYOffset);
        }
    }

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

        // 決定ボタンの描画 (自分が操作すべきフェーズの時だけ表示)
        if (data.currentPhase == BattlePhase::Select && humanIdx == data.currentTurnIdx) {
            DrawCardSelectButton(data.isHoverIdx); // 攻撃決定
        }
        if (data.currentPhase == BattlePhase::DefenseSelect && humanIdx == data.targetIdx) {
            DrawCardSelectButton(data.isHoverIdx); // 防御決定
        }
    }

    // ダメージ・回復のポップアップ描画
    DrawPopups(data);

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

        // カード画像の描画
        int picIdx = hand[i].graphicIndex;

        if (picIdx >= 0 && picIdx < CARD_KIND) {
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
        int Ele_Col = GetElementColor(hand[i].GetType());

        // テキストエリアの設定（カードのすぐ下に配置）
        int textAreaY = y + CARD_H;
        int textAreaH = 25; // テキスト背景の高さ
        bool show_box = false;  // カード下の数値を表示するかの有無

        // カテゴリ別テキスト描画
        TCHAR buf[64];
        switch (hand[i].GetCategory()) {
        case Attack:
            _stprintf_s(buf, hand[i].GetAdd() ? _T("+攻%d") : _T("攻%d"), hand[i].GetPower());
            show_box = true;
            break;
        case Bilingual:
            // 自分が攻撃ターンで、かつ攻撃フェーズの時だけ「攻」にする
            if (data.currentPhase == BattlePhase::Select && isAttackTurn) {
                _stprintf_s(buf, _T("攻%d"), hand[i].GetPower());
            }
            else {
                _stprintf_s(buf,  _T("守%d"), hand[i].GetPower());
            }
            show_box = true;
            break;
        case Magic:
            if (hand[i].GetPower() > 0) {
                _stprintf_s(buf, hand[i].GetAdd() ? _T("+攻%d") : _T("攻%d"), hand[i].GetPower());
                show_box = true;
            }
            break;
        case Defense:
            _stprintf_s(buf, _T("守%d"), hand[i].GetPower());
            show_box = true;
            break;
        case All:
            _stprintf_s(buf, _T("%d%%攻%d"), hand[i].GetPercent(), hand[i].GetPower());
            show_box = true;
            break;
        case Buy:break;
        case Sell:break;
        case Change:break;
        }

        // 手札カードの下の数値描画処理
        if (show_box) {
            DrawBox(x, textAreaY, x + CARD_W, textAreaY + textAreaH, Col.GetBoxYel(), TRUE);
            int w = GetDrawStringWidth(buf, (int)_tcslen(buf));
            DrawString(x + (CARD_W - w) / 2, textAreaY + 4, buf, Ele_Col);
        }

        // --- 選択可能かどうかの判定をロジック（BattleData）から取得 ---
        bool isSelectable = false;
        if (i < data.isCardSelectable.size()) {
            isSelectable = data.isCardSelectable[i];
        }

        // フェーズごとの使用制限（暗転）UI上書き処理
        CardCategory cat = hand[i].GetCategory();
        std::string cardName = hand[i].GetName();
        std::string defElem = hand[i].GetType(); // 手札のカード属性

        if (data.currentPhase == BattlePhase::Select && isAttackTurn) {
            // 攻撃フェーズ：防御専用カードは使用不可
            if (cat == Defense) {
                isSelectable = false;
            }
        }
        else if (data.currentPhase == BattlePhase::DefenseSelect && !isAttackTurn) {
            // 防御フェーズ：攻撃専用カード等は使用不可
            if (cat == Attack || cat == All) {
                isSelectable = false;
            }

            // 現在飛んできている攻撃の属性を取得
            std::string atkElem = data.currentAttackElement;

            // 属性相性による防御可否判定
            if (cat == Defense || cat == Bilingual) {
                // （炎属性の攻撃に、木属性の防具は燃えてしまうため使えない）
                if (atkElem == "炎" && defElem == "木") isSelectable = false;

                // ※ここに他の相性（弾けない属性など）を必要に応じて追加します
            }

            // 奇跡（魔法）の制限
            if (cat == Magic) {
                // 防御関係、弾き、跳ね返し以外は暗くする
                if (cardName != "跳ね返し" && cardName != "弾き" && cardName != "防御力アップ") {
                    // もし「回復」は防御時も使えるルールなら、ここに回復魔法の除外も足します
                    isSelectable = false;
                }

                // 弾けない属性の攻撃が来た場合の処理
                if (cardName == "弾き") {
                    // （光や闇属性の攻撃は弾けない設定）
                    if (atkElem == "光" || atkElem == "闇") {
                        isSelectable = false;
                    }
                }
            }
        }

        // 選択不可の暗転処理（そのまま）
        if ((data.currentPhase == BattlePhase::Select || data.currentPhase == BattlePhase::DefenseSelect) && !isSelectable) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
            DrawBox(x, y, x + CARD_W, y + CARD_H, Col.GetBla(), TRUE);
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

        // --- 説明文の描画 (画像の右側に改行して表示) ---
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
            // アンリミテッドのみ消費MPを今のMPにする
            if (card.GetName() == "アンリミテッド")
                DrawFormatString(textX, textY + 20, GetColor(50, 50, 255), _T("MP-%d"), player.getMp());
            else
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

    const int limitY = 390; // 手札UI（450）の手前で止める制限ライン

    // 動的なYオフセット計算
    int count = (int)data.selectedCards.size();
    // カード同士の隙間を計算
    int availableSpace = limitY - startY - CARD_H;
    int safeYOffset = (count > 1) ? (availableSpace / (count - 1)) : 40;

    // 指定されたアニメーション値と、収まるための値の小さい方を採用
    int activeYOffset = (std::min)((int)currentYOffset, safeYOffset);

    int maxDrawCount = (int)data.selectedCards.size();

    // ベースカード（1枚目）の属性を事前に取得しておく
    int baseIdx = data.selectedCards[0];
    std::string baseElement = "無";
    if (baseIdx >= 0 && baseIdx < (int)hand.size()) {
        baseElement = hand[baseIdx].GetType();
    }

    // --- 選択されたすべてのカードを縦リストとして描画 ---
    for (int i = 0; i < maxDrawCount; ++i) {
        int handIdx = data.selectedCards[i];
        if (handIdx >= 0 && handIdx < (int)hand.size()) {
            const auto& card = hand[handIdx];

            int drawX = startX;
            int drawY = startY + (i * activeYOffset);

            // 背面のテキストエリア（UIボックス）の描画
            DrawBox(drawX - 5, drawY - 5, drawX + boxWidth, drawY + CARD_H + 5, Col.GetBoxYel(), TRUE);
            DrawBox(drawX - 5, drawY - 5, drawX + boxWidth, drawY + CARD_H + 5, Col.GetBla(), FALSE);

            // カード画像の描画
            int picIdx = card.graphicIndex;
            if (picIdx >= 0 && picIdx < 100) {
                DrawExtendGraph(drawX, drawY, drawX + CARD_W, drawY + CARD_H, Pic.GetCard(picIdx), TRUE);
            }
            DrawBox(drawX, drawY, drawX + CARD_W, drawY + CARD_H, Col.GetBla(), FALSE);

            // 加算カードの属性色引き継ぎロジック
            int Ele_Col = GetElementColor(card.GetType());

            int textX = drawX + CARD_W + 15;

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
    }

    // 合計威力の表示（手札一覧の少し上に表示。ここも連動して滑らかに動きます） 
    int BaseIdx = data.selectedCards[0];
    if (BaseIdx < 0 || BaseIdx >= (int)hand.size()) return;
    CardCategory baseCat = hand[BaseIdx].GetCategory();
    if (baseCat == Healing || baseCat == MagicHealing) return;

    // 表示に必要な変数の準備
    const int totalDrawX = startX + 100;
    const int totalDrawY = 400;
    const int totalbox_x = startX;
    const int totalbox_y = 395;
    bool isAttackTurn = (player.getName() == data.Player_Turn[data.currentTurnIdx].getName());

    // 合計枠の描画
    DrawBox(totalbox_x, totalbox_y, totalbox_x + 271, totalbox_y + 25, Col.GetWhi(), TRUE);
    DrawBox(totalbox_x, totalbox_y, totalbox_x + 271, totalbox_y + 25, Col.GetBla(), FALSE);

    if (isAttackTurn) {
        // ロジック側で計算済みの最終属性をそのまま取得
        std::string effectiveElement = data.currentAttackElement;
        if (effectiveElement == "") effectiveElement = "無";

        int totalCol = GetElementColor(effectiveElement);
        DrawFormatStringToHandle(totalDrawX, totalDrawY, totalCol, Font.GetSmall(), _T("攻 %d"), data.attackTotalPower);
    }
    else {
        // 防御フェーズ
        DrawFormatStringToHandle(totalDrawX, totalDrawY, GetColor(0, 255, 255), Font.GetSmall(), _T("守 %d"), data.defenseTotalPower);
    }
}

// BattleInput がクリック判定に使うための関数
Rect BattleUIManager::GetHandCardRect(int handIndex) const {
    // --- DrawPlayerHand と完全に共通のサイズ・レイアウト設定 ---
    const float SCALE = 1.45f;                  // 拡大倍率
    const int BASE_W = 50;                      // 元のカード幅
    const int BASE_H = 50;                      // 元のカード高さ
    const int CARD_W = (int)(BASE_W * SCALE);   // 拡大後の幅
    const int CARD_H = (int)(BASE_H * SCALE);   // 拡大後の高さ

    const int Start_X = 10;                     // 1枚目のX座標
    const int Start_Y = 450;                    // 手札を表示するY座標
    const int MARGIN = 2;                       // カード同士の隙間

    const int MAX_CARDS_PER_ROW = 9;            // 1段の枚数
    const int ROW_SPACING = CARD_H + 30;        // 段ごとの縦の間隔

    // --- 各カードの行列位置を計算 ---
    int col = handIndex % MAX_CARDS_PER_ROW;
    int row = handIndex / MAX_CARDS_PER_ROW;

    // --- 描画位置と同じ座標を計算 ---
    int x = Start_X + (CARD_W + MARGIN) * col;
    int y = Start_Y + (ROW_SPACING * row);

    // 計算した矩形（左上X, 左上Y, 横幅, 縦幅）を返す
    return { x, y, CARD_W, CARD_H };
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
            Font.GetSmall());
        int drawX = x + (boxWidth - stringWidth) / 2;

        DrawCircle(x, y, r, Col.GetBla(), FALSE);
        DrawCircle(x + boxWidth, y, r, Col.GetBla(), FALSE);
        DrawBox(x, y - 10, x + boxWidth, y + 11, Col.GetBla(), FALSE);

        DrawCircle(x, y, r - 1, Col.GetWhi(), TRUE);
        DrawCircle(x + boxWidth, y, r - 1, Col.GetWhi(), TRUE);
        DrawBox(x, y - 9, x + boxWidth, y + 10, Col.GetWhi(), TRUE);

        DrawFormatStringToHandle(drawX, y - 7, GetColor(200, 50, 50), Font.GetSmall(), _T("%s"), data.Player_Turn[data.targetIdx].getName().c_str());
        // 矢印の描画（右向き）
        int startX = 295, endX = 320;
        DrawLine(startX, arrowY, endX, arrowY, Col.GetBla(), 2);
        DrawTriangle(endX, arrowY, endX - 10, arrowY - 5, endX - 10, arrowY + 5, Col.GetBla(), TRUE);
    }
    else {
        // 自傷や自己対象効果の場合（左向き）
        int startX = 320, endX = 295;
        DrawLine(startX, arrowY, endX, arrowY, Col.GetBla(), 2);
        DrawTriangle(endX, arrowY, endX + 10, arrowY - 5, endX + 10, arrowY + 5, Col.GetBla(), TRUE);
    }
}

// 防御側のカード表示
void BattleUIManager::DrawDefenseCards(const BattleData& data,
    const Player& player, int defenceIdx, float currentYOffset) const {

    // まだカードが選ばれていない場合は何もしない
    if (data.selectedDefenseCards.empty()) return;

    const auto& hand = player.Hand.GetCards();

    // 描画開始座標 (ターゲット名の下)
    int startX = 350;
    int startY = 95;
    const int limitY = 340; // 画面下限（手札エリアに被らないギリギリの位置）
    const int boxWidth = 250;
    int yOffset = (int)currentYOffset;

    // --- サイズ設定（手札より少し小さめ） ---
    const float SCALE = 1.0f;
    const int CARD_W = (int)(50 * SCALE);
    const int CARD_H = (int)(50 * SCALE);

    // --- 動的なYオフセット計算 ---
    int count = (int)data.selectedDefenseCards.size();

    // safeYOffset が正しく計算されるよう、分子を (limitY - startY - CARD_H) に
    int availableSpace = limitY - startY - CARD_H;
    int safeYOffset = (count > 1) ? (availableSpace / (count - 1)) : 40;
    // 指定されたアニメーション値と、収まるための値の小さい方を採用
    int activeYOffset = (std::min)((int)currentYOffset, safeYOffset);

    // 選択フェーズなら全表示、演出フェーズならカウント分だけ表示
    int maxDrawCount = (int)data.selectedDefenseCards.size(); // 基本は全表示
    if (data.currentPhase == BattlePhase::DefenseReveal) {
        // 演出中のみカウントによる制限をかける
        maxDrawCount = (std::min)((int)data.selectedDefenseCards.size(), data.animDefenseCardCount);
    }

    for (int i = 0; i < maxDrawCount; ++i) {
        int handIdx = data.selectedDefenseCards[i];
        if (handIdx >= 0 && handIdx < (int)hand.size()) {
            const auto& card = hand[handIdx];
            int drawX = startX;
            int drawY = startY + (i * activeYOffset);

            // 背面のテキストエリアの描画
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
            int Ele_Col = GetElementColor(card.GetType());

            // カテゴリごとの文字描画
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
        // =============================================================
        // 合計防御力の表示
        // =============================================================

        // ロジック側で計算済みの最終属性をそのまま取得
        std::string effectiveDefElement = data.currentDefenseElement;
        if (effectiveDefElement == "") effectiveDefElement = "無";

        int totalEleCol = GetElementColor(effectiveDefElement);

        int totalDrawX = startX + 100;
        int totalDrawY = 400;
        const int totalbox_x = 340;
        const int totalbox_y = 395;

        DrawBox(totalbox_x, totalbox_y, totalbox_x + 271, totalbox_y + 25, Col.GetWhi(), TRUE);
        DrawBox(totalbox_x, totalbox_y, totalbox_x + 271, totalbox_y + 25, Col.GetBla(), FALSE);

        DrawFormatStringToHandle(totalDrawX, totalDrawY, totalEleCol, Font.GetSmall(), _T("守 %d"), data.defenseTotalPower);
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

// バトル終了画面の描画
void BattleUIManager::DrawEndScreen(const BattleData& data) const {
    // 中央のウィンドウ
    DrawBox(200, 150, 800, 450, Col.GetBla(), TRUE);
    DrawBox(205, 155, 795, 445, Col.GetCurYel(), TRUE);

    // 生き残ったプレイヤー（勝者）を探す
    std::string winnerName = "DRAW";
    for (const auto& p : data.Player_Turn) {
        if (p.getHp() > 0) {
            winnerName = p.getName();
            break;
        }
    }

    // 勝利メッセージ
    std::string resultText = winnerName + " WIN!!";
    int w = GetDrawStringWidth(resultText.c_str(), (int)resultText.length());
    DrawFormatString(500 - (w / 2), 250, Col.GetRed(), "%s", resultText.c_str());

    // 戻るボタンの描画 (既存のボタン位置に合わせるか、中央に配置)
    unsigned int color = data.isHoverIdx[BattleOption::RETURN] ? Col.GetSky() : Col.GetWhi();
    DrawBox(400, 350, 600, 400, color, TRUE);
    DrawBox(400, 350, 600, 400, Col.GetBla(), FALSE);
    DrawString(460, 365, _T("バトル設定へ戻る"), Col.GetBla());
}

// ダメージ・回復のポップアップ描画
void BattleUIManager::DrawPopups(const BattleData& data) const {
    const int startX = 660; // プレイヤーステータスの少し左
    const int startY = 75;
    const int marginY = 40;

    for (const auto& popup : data.popups) {
        if (popup.playerIdx >= 0 && popup.playerIdx < data.Player_Turn.size()) {
            int x = startX;
            // timerやoffsetYを使って、上にフワッと昇るアニメーションを表現
            int y = startY + (popup.playerIdx * marginY) - popup.offsetY;

            // アウトライン（縁取り）を描画して視認性を高める
            DrawFormatString(x - 1, y - 1, GetColor(0, 0, 0), "%s", popup.text.c_str());
            DrawFormatString(x + 1, y - 1, GetColor(0, 0, 0), "%s", popup.text.c_str());
            DrawFormatString(x - 1, y + 1, GetColor(0, 0, 0), "%s", popup.text.c_str());
            DrawFormatString(x + 1, y + 1, GetColor(0, 0, 0), "%s", popup.text.c_str());

            // 本体のテキストを描画
            DrawFormatString(x, y, popup.color, "%s", popup.text.c_str());
        }
    }
}