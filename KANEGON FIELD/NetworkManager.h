// 通信処理用クラス

#pragma once
// ネットワーク用に固定長構造体に修正
#define MAX_NAME_LEN 32

#include "DxLib.h"
#include <string>
#include <queue> // キューを使うために追加

// 通信コマンド（今回はフェーズ同期のみ）
enum class CommandType {
    SYNC_PHASE,
    START_BATTLE,
    SELECT_TEAM,    // チーム選択通知
    DISCONNECT,     // 切断通知
    SYNC_LOBBY,     //全員の情報を一括で送るコマンド
};

// ゲーム用のパケット構造体（固定長）
struct GamePacket {
    CommandType type;
    int teamId;                // 選んだチーム番号を送るため
    char playerName[MAX_NAME_LEN];   // 誰が選んだかを送るため
    int value1;                // フェーズ番号
};

class NetworkManager {
private:
    std::vector<int> clientHandles; // ホスト用：接続してきたクライアントのリスト
    int hostHandle = -1;            // クライアント用：接続先のホストハンドル
    int netHandle = -1;
    bool isHost = false;
    bool isConnected = false;
    std::queue<GamePacket> packetQueue; // 受信したデータを溜める箱

public:
    NetworkManager();
    ~NetworkManager();

    // 全員に送る関数
    void BroadcastPacket(const GamePacket& packet);

    // 特定の人に送る関数
    void SendPacketTo(int handle, const GamePacket& packet);

    // ホストとして接続待機を開始する (port: ポート番号。例: 9850)
    bool StartHost(int port);

    // 外から参照用
    std::vector<int>& GetClientHandles() { return clientHandles; }

    // クライアントとしてホストに接続する (ipAddress: 相手のIP, port: ポート番号)
    bool ConnectAsClient(const std::string& ipAddress, int port);

    // 毎フレーム呼ぶ更新処理（接続待ちや、データの受信チェック）
    void Update();

    // データを送信する
    void SendPacket(const GamePacket& packet);

    // 通信を切断する
    void Disconnect();

    // 接続状態の取得
    bool IsConnected() const { return isConnected; }
    bool IsHost() const { return isHost; } // ホストかどうかの判定を追加

    // 溜まった受信データを取り出す関数
    bool PopPacket(GamePacket& outPacket);

    // 現在接続しているクライアントの数を取得
    size_t GetClientCount() const { return clientHandles.size(); }

    // 最後に接続してきたクライアントのハンドルを取得（同期用）
    int GetLastAddedClient() const {
        if (clientHandles.empty()) return -1;
        return clientHandles.back();
    }
};