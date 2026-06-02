// マウス、キー入力判定の関数
#pragma once

#pragma once
#include "DxLib.h"

class InputManager {
private:
    int mouseX, mouseY;         // マウスの座標
    int currentMouseInput;      // 今フレームのマウスのボタン管理変数
    int prevMouseInput;         // 前回どのボタンが押されたかの状態格納

public:
    // コンストラクタで初期化
    InputManager() : mouseX(0), mouseY(0), currentMouseInput(0), prevMouseInput(0) {}

    // 更新処理
    void Update();

    // --- マウス情報取得関数（ゲッター） ---

    int GetMouseX() const { return mouseX; }
    int GetMouseY() const { return mouseY; }

    // 左クリックされた瞬間判定
    bool IsLeftClicked() const;

    // 特定の範囲内にマウスがあるかを判定
    bool IsMouseOver(int x, int y, int w, int h) const;
};

