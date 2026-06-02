#include "InputManager.h"

// 更新処理
void InputManager::Update() {
    // 前フレームの入力を保存
    prevMouseInput = currentMouseInput;

    // 現在の入力を取得
    GetMousePoint(&mouseX, &mouseY);
    currentMouseInput = GetMouseInput();
}

// 右クリック判定
bool InputManager::IsLeftClicked() const {
    return (currentMouseInput & MOUSE_INPUT_LEFT) && !(prevMouseInput & MOUSE_INPUT_LEFT);
}

// 特定の範囲内にマウスがあるかを判定
bool InputManager::IsMouseOver(int x, int y, int w, int h) const {
    return (mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h);
}
