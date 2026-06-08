#include "Picture.h"
#include "DxLib.h"
#include <assert.h> // 安全チェック用

// コンストラクタ
Picture::Picture() : Tit(-1), Tit_Button(-1), Sel(-1), Start_Button(-1), Solo_Button(-1), AI_Button(-1), Member(-1), Bat(-1)
{
	for (int i = 0; i < 3; ++i) Sel_Button[i] = -1;
	for (int i = 0; i < 4; ++i) Team_Button[i] = -1;
	for (int i = 0; i < CARD_KIND; ++i) Card[i] = -1;
}

// デストラクタ
Picture::~Picture() {
	Release();
}

// 読み込み関数
void Picture::Read() {
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
void Picture::Release() {
	// InitGraph() は、DXライブラリで読み込んだすべての画像をメモリから消去する便利関数です
	InitGraph();
}

// 画像の上にマウスが置かれたら色が少し灰色になる関数
void Picture::MouseHoverDraw(int x, int y, int img) const {
	SetDrawBright(180, 180, 180);
	DrawGraph(x, y, img, TRUE);
	SetDrawBright(255, 255, 255);
}

// ボタンが使えない時に色が変わる関数
void Picture::ButtonRockDraw(int x, int y, int img) const {
	SetDrawBright(100, 100, 100);
	DrawGraph(x, y, img, TRUE);
	SetDrawBright(255, 255, 255);
}

// メモリ上にPicを作成
Picture Pic;