#include "SceneManager.h"

// 各シーンのヘッダ
#include "TitleScene.h"
#include "SelectScene.h"

// 新しくしたSettingSceneの拡張シーン
#include "BaseLobbyScene.h" 
#include "TrainingLobbyScene.h"
#include "BrawlLobbyScene.h"
#include "RankedLobbyScene.h"

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
    case SceneName::SETTING:
        // 選ばれたモードに合わせて、作る「子クラス」を変える！
        if (g_selectedMode == SelectScene::Option::TRANING) {
            return std::make_unique<TrainingLobbyScene>();
        }
        else if (g_selectedMode == SelectScene::Option::PVP) {
            return std::make_unique<BrawlLobbyScene>();
        }
        else {
            return std::make_unique<RankedLobbyScene>();
        }
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
            // 【変更点②】SettingScene ではなく、親の BaseLobbyScene にキャストする！
            auto* lobbyScene = dynamic_cast<BaseLobbyScene*>(m_currentScene.get());
            if (lobbyScene) {
                // 子クラスが修行だろうが乱闘だろうが、親の機能を使ってデータを取り出せる
                carryOverPlayers = lobbyScene->GetBattlePlayers();
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