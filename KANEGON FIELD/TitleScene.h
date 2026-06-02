// タイトル画面クラス
#pragma once

#include "InterfaceScene.h"
#include "InputManager.h"
#include <string>

class TitleScene :public IScene
{
private:
    int inputHandle = -1;   // DXライブラリの文字入力ハンドル
    bool isHover = false;      // 入力ボックスの上にマウスがあるか
    bool isStartHover = false; // スタートボタンの上にマウスがあるか
    bool isFocused = false;    // 入力ボックスにフォーカスが当たっているか

public:
    // コンストラクタ
    TitleScene();

    // デストラクタ
    ~TitleScene() override = default;

    // 初期化
    void Init();

    // 【新設計】戻り値を SceneName に、引数を InputManager に変更
    SceneName Update(const InputManager& input) override;

    // 【新設計】後ろに const を付与
    void Draw() const override;

    // 【安全対策】Drawから呼ばれるため、この関数にも const を付与
    std::string GetName() const;
};

