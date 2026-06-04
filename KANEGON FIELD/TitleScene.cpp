#include "TitleScene.h"
#include "Picture.h"
#include "DxLib.h"
#include "GameConfig.h"

TitleScene::TitleScene() {
    // タイトル画面用の画像読み込みなどがあればここで行う
}

TitleScene::~TitleScene() {
    // 読み込んだ画像の削除など
}

// 初期化関数
void TitleScene::Init() {
    if (inputHandle == -1) {
        // ※日本語（漢字）を入力したい場合は、第2引数を TRUE にしてください
        inputHandle = MakeKeyInput(16, FALSE, FALSE, FALSE);

        // 各種色を設定（黒文字、白背景、白カーソルなど）
        SetKeyInputStringColor(
            inputHandle,
            Col.GetBla(),
            GetColor(150, 150, 150),
            Col.GetBla(),
            Col.GetWhi(),
            Col.GetWhi(),
            Col.GetRed(),
            GetColor(150, 150, 150), 
            Col.GetWhi(),
            Col.GetWhi()
        );
    }
}

SceneName TitleScene::Update(const InputManager& input) {

    // 1. InputManagerの機能を使って、指定の矩形内にマウスがあるか判定
    isHover = input.IsMouseOver(350, 375, 300, 50);
    isStartHover = input.IsMouseOver(350, 430, 300, 150);

    // 2. 左クリックされた瞬間の処理
    if (input.IsLeftClicked()) {
        if (isHover) {
            isFocused = true;
            SetActiveKeyInput(inputHandle); // 入力欄をアクティブ化
        }
        else if (isStartHover) {
            // スタートボタンが押されたら「モード選択画面」へ遷移を要求
            return SceneName::SELECT;
        }
        else {
            isFocused = false;
            SetActiveKeyInput(-1); // ボックス外クリックで入力を非アクティブ化
        }
    }

    // 3. キーボードによる文字入力確定の判定
    if (isFocused) {
        int state = CheckKeyInput(inputHandle);
        if (state == 1) { // Enterキーで確定
            isFocused = false;
            return SceneName::SELECT; // 確定と同時に「モード選択画面」へ
        }
    }

    // 4. シーンを切り替えない場合は、自分自身(TitleScene)を返して現状維持
    return SceneName::TITLE;
}

void TitleScene::Draw() const {
    int NameBox_x = 350;
    int NameBox_y = 375;
    int NameBoxEnd_x = 650;
    int NameBoxEnd_y = 425;

    DrawGraph(START_X, START_Y, Pic.GetTit(), TRUE);

    // ボックス描画
    unsigned int boxColor = isFocused ? Col.GetCurYel() : Col.GetWhi();
    DrawBox(NameBox_x, NameBox_y, NameBoxEnd_x, NameBoxEnd_y, boxColor, TRUE);
    DrawBox(NameBox_x, NameBox_y, NameBoxEnd_x, NameBoxEnd_y, Col.GetBla(), FALSE);

    // 現在入力されている名前を取得（GetNameがconstなので呼べる）
    std::string currentName = GetName();

    int Button_y = 430;
    // ボタンの描画（マウスホバーで画像変更）
    if (isStartHover) {
        Pic.MouseHoverDraw(NameBox_x, Button_y + 1, Pic.GetTitButton());
    }
    else {
        DrawGraph(NameBox_x, Button_y, Pic.GetTitButton(), TRUE);
    }

    // 名前入力欄の文字表示の原点座標
    int Name_x = 355, Name_y = 390;

    // 文字列の表示
    if (isFocused) {
        DrawKeyInputString(Name_x, Name_y, inputHandle);
    }
    else {
        // 未入力なら案内、入力済みならその名前を出す
        std::string user_name = currentName.empty() ? _T("ここをクリックして名前入力") : currentName;
        unsigned int fontColor = currentName.empty() ? GetColor(150, 150, 150) : Col.GetWhi();
        DrawString(Name_x, Name_y, user_name.c_str(), fontColor);
    }
}