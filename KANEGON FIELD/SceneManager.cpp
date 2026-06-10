#include "SceneManager.h"

// 各シーンのヘッダ
#include "TitleScene.h"
#include "SelectScene.h"
#include "SettingScene.h"
#include "BattleScene.h"

// コンストラクタ
SceneManager::SceneManager(SceneName startScene) : m_currentName(startScene) {
    m_currentScene = CreateScene(m_currentName);
}

// 指定したシーンを新しく生成する関数
std::unique_ptr<IScene> SceneManager::CreateScene(SceneName name) {
    
    // switchでシーンの分岐をする
    switch(name){
    case SceneName::TITLE:return std::make_unique<TitleScene>();
    case SceneName::SELECT:return std::make_unique<SelectScene>();
    case SceneName::SETTING:return std::make_unique<SettingScene>(g_selectedMode);
    case SceneName::BATTLE:return std::make_unique<BattleScene>();
    default:
        return nullptr;
    }
}

// 更新処理
void SceneManager::Update(const InputManager& input) {
    if (!m_currentScene)return;

    // 現在のシーンを更新し、次のシーンの要望を受け取る
    SceneName nextName = m_currentScene->Update(input);

    // 要望されたシーンが現在のシーンと異なる場合、シーンを切り替える
    if (nextName != m_currentName && nextName != SceneName::NONE) {

        // 【追加】切り替え時にデータを一時保持するための変数
        std::vector<Player> carryOverPlayers;

        // 設定画面からバトル画面へ移動するとき、プレイヤー情報を抜き出す
        if (m_currentName == SceneName::SETTING && nextName == SceneName::BATTLE) {
            auto* settingScene = dynamic_cast<SettingScene*>(m_currentScene.get());
            if (settingScene) {
                // ※ SettingSceneに用意したゲッターからメンバー変数を取得
                carryOverPlayers = settingScene->GetBattlePlayers();
            }
        }

        // 次のシーンの名前を更新して生成
        m_currentName = nextName;
        m_currentScene = CreateScene(m_currentName);

        // 新しいシーンがバトル画面なら、初期化関数にデータを渡す
        if (m_currentName == SceneName::BATTLE) {
            auto* battleScene = dynamic_cast<BattleScene*>(m_currentScene.get());
            if (battleScene) {
                battleScene->Initialize(carryOverPlayers);
            }
        }
    }
}

// --- 描画処理 ---
void SceneManager::Draw() const {
    if (m_currentScene) {
        m_currentScene->Draw();
    }
}