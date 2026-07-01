/*
* 先を見据えすぎてしまい切羽詰まった為、再構築することにした
* 改めてこのプロフラムでやりたいことメモ
*
*　【このゲームを作成する上での最終目標】
* 　既存ゲーム「ＧＯＤ ＦＩＥＬＤ」の仕組みをほぼ完全再現＆
* 　多少のオリジナリティを出したオンライン対戦型カードゲームの制作
*
*　【必ず導入したい仕組み】
*　 ・データベースでのカード導入
* 　・それぞれの設定画面で各々別の処理をする
*   ・できる限りの関数化、クラス化
*　 ・AI対戦をスムーズに行えるようにする
*　 ・カウンターやランダム効果、ランダムターゲットやターゲットの指定など細かい機能を組む
* 　・見やすいUIの作成
* 
*　【可能なら】
* 　・サーバーに接続してオンラインでの対戦を可能とする
*	・
* 
*　【余談】
*　 発想の由来は、友人の何気ない発言から誕生した「かねごん集」が元ネタとなっている
* 　
*/

// インクルード系統
#include <iostream>
#include <string>
#include <random>
#include <vector>
#include "DxLib.h"
#include "GameConfig.h"
#include "Picture.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "CardDatabase.h"
#include "NetworkManager.h"

// クラスのインスタンス化
ColorManager Col;
FontManager Font;

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

    SetDoubleStartValidFlag(TRUE);  // 多重起動を許可する（2枚開けるようになる）
    SetAlwaysRunFlag(TRUE);         // 非アクティブ状態でも処理を止めない（通信の受信に必須）

    ChangeWindowMode(TRUE);
    if (DxLib_Init() == -1) return -1;
    SetWindowText(_T("KANEGON FIELD"));         // ウィンドウのテキスト変更
    SetGraphMode(WIN_MAX_X, WIN_MAX_Y, 32);     // ウィンドウのサイズ変更
    SetBackgroundColor(255, 255, 255);			// 背景色設定
    SetDrawScreen(DX_SCREEN_BACK);

    // 色の読み込み
    Col.Init();

    // フォントの読み込み
    Font.Init();

    // 画像の読み込み
    Pic.Read();

    // カードCSVの読み込みとエラー処理
    const std::string CSV_PATH = "./data/CSV/card_data.csv"; // ※実際のパスに合わせてください
    if (!CardDB.Load(CSV_PATH)) {
        // 失敗したら画面にメッセージを出して止める
        printfDx("エラー: %s が見つかりません！\n", CSV_PATH.c_str());
        ScreenFlip();
        WaitKey();
        DxLib_End();
        return -1;
    }

    // 通信マネージャーのインスタンス化
    NetworkManager netManager;

    // マネージャーの生成
    InputManager inputManager;

    // 初期シーンを「タイトル」に指定してシーンマネージャーを作成
    SceneManager sceneManager(SceneName::TITLE, &netManager);

    while (
        ClearDrawScreen() == 0 &&		// 画面に描かれたものを消去する
        ProcessMessage() == 0 &&        // ウィンドウズのメッセージ処理
        CheckHitKey(KEY_INPUT_ESCAPE) == 0)
    {
        // 入力状態の更新
        inputManager.Update();

        // ゲームの更新処理
        sceneManager.Update(inputManager);

        // ゲームの描画処理
        sceneManager.Draw();

        // 全背景を消す
        ScreenFlip();
    }

    // ゲームコンフィグ内の関数の削除
    Font.End();

    DxLib_End();
    return 0;
}