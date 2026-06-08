// 戦闘(ゲームプレイ)画面クラス
#pragma once
#include "InterfaceScene.h"
#include <vector>
#include "Player.h"

class BattleScene :public IScene
{
private:
	std::vector<Player>Player_Turn;			// プレイヤーのターンを分別する変数
	int currentTurnIdx;						// 現在のターンプレイヤー添字
	int selectedOption = NONE;				// 現在選ばれている選択肢
	int selectCard = -1;					// 現在選ばれてるカード
	std::vector<int> selectedCards;			// 選んだ手札のインデックスを順番に格納
	std::vector<int> selectedDefenseCards;	// 防御側が選択したカードのインデックス
	int totalPower = 0;						// 重ね掛けした合計威力
	int selectPlayer;						// 現在選ばれてるプレイヤー
	bool isHoverIdx[MAX];					// 各ボタンの上にマウスがあるか
	bool isHoverCardIdx[CARD_MAX];			// カード枠の上にマウスがあるか
	bool isHoverPlayerIdx[PLAYER_MAX];		// どのプレイヤー枠の上にマウスがあるか
	bool playerTarget = false;				// プレイヤーを指定したかどうか
	int hoveredCardIdx;						// マウスカーソルで選択しているカード番号
	int targetIdx;							// マウスでホバーしたり選択した相手の番号
	bool isSurrenderConfirm;				// あきらめる確認ウィンドウが開いているか

public:
	// コンストラクタとデストラクタ
	BattleScene();
	~BattleScene()override;

	// ISceneの仮想関数をオーバーライド
	SceneName Update(const InputManager& input) override;
	void Draw() const override;
};