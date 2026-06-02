#include "TitleScene.h"
#include "DxLib.h"

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
            GetColor(0, 0, 0), 
            GetColor(150, 150, 150),
            GetColor(0, 0, 0), 
            GetColor(255, 255, 255), 
            GetColor(255, 255, 255),
            GetColor(255, 0, 0), 
            GetColor(150, 150, 150), 
            GetColor(255, 255, 255),
            GetColor(255, 255, 255)
        );
    }
}

SceneName TitleScene::Update(const InputManager& input) {

    // 何も入力がなければ、自分自身（Title）を維持する
    return SceneName::TITLE; // ※修正: 戻り値は SceneName::Title にします
}

void TitleScene::Draw() const {
    // 画面中央付近に文字を描画
    DrawString(380, 350, _T("ミステリアス リンネキューブ"), GetColor(255, 255, 255));
    DrawString(360, 450, _T("Press Enter to Start"), GetColor(150, 150, 150));
}