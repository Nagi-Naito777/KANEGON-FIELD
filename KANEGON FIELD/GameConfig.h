// ゲーム関係に使う細かい数値や関数の指定場所
#pragma once

// ゲーム画面の原点マクロ
#define START_X 0
#define START_Y 0

//画面サイズ指定マクロ
#define WIN_MAX_X 1000
#define WIN_MAX_Y 800

// 戻るボタンの座標の原点
#define RET_BUT_X 10
#define RET_BUT_Y 10

// 戻るボタンの座標の終点
#define RET_BUT_END_X 100
#define RET_BUT_END_Y 40

// 自分自身の名前表示座標の原点
#define NAME_START_Y 770

// セレクト画面のボタンのX座標の開始点
#define SELBUT_START_X 200

// 最大ゲーム可能人数
#define MEMBER_MAX 9

// フォント管理クラス
class FontManager
{
private:
    int m_smallHandle;
    int m_normalHandle;
    int m_bigHandle;

public:
    // コンストラクタ
    FontManager() : m_smallHandle(-1), m_normalHandle(-1), m_bigHandle(-1) {}
    ~FontManager() = default;

    // フォント初期化
    void Init() {
        m_smallHandle = CreateFontToHandle(_T("MS ゴシック"), 16, 3);
        m_normalHandle = CreateFontToHandle(_T("MS ゴシック"), 32, 3);
        m_bigHandle = CreateFontToHandle(_T("MS ゴシック"), 64, 5);
    }

    // フォントデータ削除
    void End() {
        DeleteFontToHandle(m_smallHandle);
        DeleteFontToHandle(m_normalHandle);
        DeleteFontToHandle(m_bigHandle);
    }

    // --- ゲッター関数 ---
    int GetSmall() const { return m_smallHandle; }
    int GetNormal() const { return m_normalHandle; }
    int GetBig() const { return m_bigHandle; }
};

class ColorManager {
private:
    int m_red;    // 赤
    int m_blu;    // 青
    int m_gre;    // 緑
    int m_yel;    // 黄色
    int m_bla;    // 黒
    int m_whi;    // 白
    int m_gra;    // 灰色
    int m_curyel; // カーソルが重なった時の黄色
    int m_sky;    // 水色
public:
    // コンストラクタ
    ColorManager()
        : m_red(0)
        , m_blu(0)
        , m_gre(0)
        , m_yel(0)
        , m_bla(0)
        , m_whi(0)
        , m_gra(0)
        , m_curyel(0)
        , m_sky(0)
    {}

    // 初期化処理
    void Init() {
        m_red = GetColor(255, 0, 0);    // 赤
        m_blu = GetColor(0, 0, 255);    // 青
        m_gre = GetColor(0, 255, 0);    // 緑
        m_yel = GetColor(255, 255, 0);  // 黄色
        m_bla = GetColor(0, 0, 0);      // 黒
        m_whi = GetColor(255, 255, 255);// 白
        m_gra = GetColor(128, 128, 128);// 灰色
        m_curyel = GetColor(255, 255, 100);
        m_sky = GetColor(0, 255, 255);
    }

    // --- ゲッター ---
    int GetRed() const { return m_red; }
    int GetBlu() const { return m_blu; }
    int GetGre() const { return m_gre; }
    int GetYel() const { return m_yel; }
    int GetBla() const { return m_bla; }
    int GetWhi() const { return m_whi; }
    int GetGra() const { return m_gra; }
    int GetCurYel()const { return m_curyel; }
    int GetSky() const { return m_sky; }
};

extern ColorManager Col;
extern FontManager Font;