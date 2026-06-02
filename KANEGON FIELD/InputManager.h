#pragma once
// マウス、キー入力判定の関数

#pragma once
#include "DxLib.h"

class InputManager {
private:
    int mouseX, mouseY;
    int currentMouseInput;
    int prevMouseInput;

public:
    // コンストラクタで初期化
    InputManager() : mouseX(0), mouseY(0), currentMouseInput(0), prevMouseInput(0) {}

    // 毎フレームの最初に1回だけ呼ぶ更新処理
    void Update() {
        // 前フレームの入力を保存
        prevMouseInput = currentMouseInput;

        // 現在の入力を取得
        GetMousePoint(&mouseX, &mouseY);
        currentMouseInput = GetMouseInput();
    }

    // --- マウス情報取得関数（ゲッター） ---

    int GetMouseX() const { return mouseX; }
    int GetMouseY() const { return mouseY; }

    // 左クリックされた「瞬間」か（トリガー判定）
    bool IsLeftClicked() const {
        return (currentMouseInput & MOUSE_INPUT_LEFT) && !(prevMouseInput & MOUSE_INPUT_LEFT);
    }

    // 左クリックが「押されている」か（プレス判定・追加機能）
    bool IsLeftPressed() const {
        return (currentMouseInput & MOUSE_INPUT_LEFT) != 0;
    }

    // 特定の範囲内にマウスがあるかを判定
    bool IsMouseOver(int x, int y, int w, int h) const {
        return (mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h);
    }
};

