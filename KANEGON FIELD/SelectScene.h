// 戦闘モードの選択画面クラス
#pragma once
#include "InterfaceScene.h"
#include "Player.h" // g_player を使うためにインクルード

class SelectScene : public IScene
{
public:
    enum Option {
        NONE = -1,      // 何も選択されてない
        TRANING,        // 修行モード
        PVP,            // プレイヤー乱闘モード
        TAIMAN,         // 1vs1モード
        RETURN,         // タイトルに戻る
        MAX             // モード選択最大数
    };

    SelectScene();
    ~SelectScene() override;

    SceneName Update(const InputManager& input) override;
    void Draw() const override;
    // 各シーンのクラス定義に追加
    void SetNetworkManager(NetworkManager* net) override {} // 何もしなくてOK

private:
    int selectedOption;         // 現在選ばれている選択肢
    bool isHoverIdx[MAX];       // 各ボタンの上にマウスがあるか
};

extern SelectScene::Option g_selectedMode;