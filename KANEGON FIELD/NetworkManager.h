// 通信処理用クラス

#pragma once
#include "DxLib.h"
#include <string>
#include <queue> // キューを使うために追加

// 通信コマンド（今回はフェーズ同期のみ）
enum class CommandType {
    SYNC_PHASE,
    START_BATTLE,
};

// ゲーム用のパケット構造体（固定長）
struct GamePacket {
    CommandType type;
    int value1; // 汎用的な数値（今回はフェーズの番号を入れる）
};

class NetworkManager {
private:
    int netHandle;
    bool isHost;
    bool isConnected;
    std::queue<GamePacket> packetQueue; // 受信したデータを溜める箱

public:
    NetworkManager();
    ~NetworkManager();

    // ホストとして接続待機を開始する (port: ポート番号。例: 9850)
    bool StartHost(int port);

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
};