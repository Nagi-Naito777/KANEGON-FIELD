// 写真ロードクラス
#pragma once
#include "DxLib.h"

//カードサイズ
#define CARD_CELL 50

//カードの合計種類数
#define CARD_KIND 400

class Picture
{
public:
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

	Picture()
		: Tit(-1)
		, Tit_Button(-1)
		, Sel(-1)
		, Start_Button(-1)
		, Solo_Button(-1)
		, AI_Button(-1)
		, Member(-1)
		, Bat(-1)
	{
		// 配列の要素は「初期化子リスト（上の部分）」に書けないため、ここでループ初期化します
		for (int i = 0; i < 3; ++i) {
			Sel_Button[i] = -1;
		}
		for (int i = 0; i < 4; ++i) {
			Team_Button[i] = -1;
		}
		for (int i = 0; i < CARD_KIND; ++i) {
			Card[i] = -1;
		}
	}

	// デストラクタ（クラスが消滅する時に自動で画像を解放）
	~Picture() {
		Release();
	}

	void Read() {
		// タイトル関係
		Tit = LoadGraph(_T("./data/resource/Title.png"));
		Tit_Button = LoadGraph(_T("./data/resource/Title_Button.png"));
		// セレクト関係
		Sel = LoadGraph(_T("./data/resource/BackGround.png"));
		LoadDivGraph(_T("./data/resource/Select_Button.png"), 3, 1, 3, 600, 104, Sel_Button);
		// オプション関係
		Solo_Button = LoadGraph(_T("./data/resource/Solo_Button.png"));
		LoadDivGraph(_T("./data/resource/Team_Button.png"), 4, 1, 4, 200, 104, Team_Button);
		Start_Button = LoadGraph(_T("./data/resource/Start_Button.png"));
		AI_Button = LoadGraph(_T("./data/resource/AI_Button.png"));
		Member = LoadGraph(_T("./data/resource/Member_Button.png"));
		// バトル関係
		Bat = LoadGraph(_T("./data/resource/GreenBack.png"));
		LoadDivGraph(_T("./data/resource/Card.png"), CARD_KIND, 20, 20, CARD_CELL, CARD_CELL, Card);
	}

	// 全画像のメモリ解放関数
	void Release() {
		// InitGraph() は、DXライブラリで読み込んだすべての画像をメモリから消去する便利関数です
		InitGraph();
	}

	// 画像の上にマウスが置かれたら色が少し灰色になる関数
	void MouseHoverDraw(int x, int y, int img) {
		SetDrawBright(180, 180, 180);
		DrawGraph(x, y, img, TRUE);
		SetDrawBright(255, 255, 255);
	}
};

extern Picture Pic; // 変数のみ宣言