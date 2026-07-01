// ゲームシーン管理クラス
#pragma once

// メモリの自動管理用ヘッダー
#include <memory>
// シーンの共通ルール(IScene)と名前(SceneName)が定義されているヘッダー
#include "InterfaceScene.h"

class SceneManager {
private:
    // 現在表示しているシーンの実体
    std::unique_ptr<IScene> m_currentScene;

    // 現在のシーンの種類
    SceneName m_currentName;

    // ネットワーク関係の保持ポインタ
    NetworkManager* m_netManager;

    // 指定したシーンを新しく生成する関数
    std::unique_ptr<IScene> CreateScene(SceneName name);

public:
    // コンストラクタ
    SceneManager(SceneName startScene, NetworkManager* net);

    // 更新処理
    void Update(const InputManager& input);

    // 描画処理
    void Draw() const;
};