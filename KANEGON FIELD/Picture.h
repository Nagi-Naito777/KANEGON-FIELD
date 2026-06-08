// 写真ロードクラス
#pragma once
#include "DxLib.h"
#include <assert.h> // 安全チェック用

//カードサイズ
#define CARD_CELL 50

//カードの合計種類数
#define CARD_KIND 400

class Picture
{
private:
	// 画像を格納する変数達
	int Tit;			// タイトルシーン画像
	int Tit_Button;		// タイトルの開始ボタン画像
	int Sel;			// モードセレクトシーン画像
	int Sel_Button[3];	// セレクト画面のボタン画像配列
	int Start_Button;	// スタートボタン画像
	int Team_Button[4];	// チームカラー分けボタン画像
	int Solo_Button;	// 個人参戦ボタン画像
	int AI_Button;		// 修行モード時のAI人数変更ボタン画像
	int Member;			// 修行モード時の対戦人数を選択するボタン画像
	int Bat;			// 戦闘シーン画像
	int Card[CARD_KIND];// カード画像用の配列

public:
	// コンストラクタ
	Picture();

	// デストラクタ
	~Picture();

	// 読み込み関数
	void Read();

	// 全画像のメモリ解放関数
	void Release();

	// 画像の上にマウスが置かれたら色が少し灰色になる関数
	void MouseHoverDraw(int x, int y, int img) const;

	// ボタンが使えない時に色が変わる関数
	void ButtonRockDraw(int x, int y, int img) const;

	// ゲッター関数
	int GetTit() const { return Tit; }
	int GetTitButton() const { return Tit_Button; }
	int GetSel() const { return Sel; }

	int GetStartButton() const { return Start_Button; }
	int GetSoloButton() const { return Solo_Button; }
	int GetAIButton() const { return AI_Button; }
	int GetMember() const { return Member; }
	int GetBat() const { return Bat; }

	// 配列の場合は、添え字（何番目か）を指定して受け取る
	int GetSelButton(int index) const {
		assert(index >= 0 && index < 3);
		return Sel_Button[index];
	}

	int GetTeamButton(int index) const {
		assert(index >= 0 && index < 4);
		return Team_Button[index];
	}

	int GetCard(int index) const {
		assert(index >= 0 && index < CARD_KIND);
		return Card[index];
	}
};

extern Picture Pic; // 変数のみ宣言