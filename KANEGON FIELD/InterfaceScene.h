#pragma once
#include "InputManager.h"

enum class SceneName {
	NONE = -1,	// 未定義
	TITLE,		// タイトル画面
	SELECT,		// ステージ選択画面
	SETTING,	// ゲーム設定画面
	BATTLE,		// ゲームプレイ画面
};

class IScene {
public:
	// 仮想関数(virtual)による上書き前提関数の作成
	virtual ~IScene() {}

	// 毎フレームの更新処理
	virtual SceneName Update(const InputManager& input) = 0;

	// 毎フレームの描画処理
	virtual void Draw()const = 0;
};

