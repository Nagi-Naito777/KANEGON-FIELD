#include "NetworkManager.h"

NetworkManager::NetworkManager() : netHandle(-1), isHost(false), isConnected(false) {
}

NetworkManager::~NetworkManager() {
    Disconnect(); // プログラム終了時などに確実に切断する
}

bool NetworkManager::StartHost(int port) {
    // 接続待機を開始
    if (PreparationListenNetWork(port) == -1) {
        return false; // 待機失敗（既にポートが使われているなど）
    }
    isHost = true;
    return true;
}

bool NetworkManager::ConnectAsClient(const std::string& ipAddress, int port) {
    // 文字列のIPアドレス(例:"192.168.1.5")をDxLibのIPDATA型に変換
    IPDATA ip;
    int p1, p2, p3, p4;
    if (sscanf_s(ipAddress.c_str(), "%d.%d.%d.%d", &p1, &p2, &p3, &p4) != 4) {
        return false; // IPアドレスの形式エラー
    }
    ip.d1 = p1; ip.d2 = p2; ip.d3 = p3; ip.d4 = p4;

    // ホストへ接続を試みる
    netHandle = ConnectNetWork(ip, port);
    if (netHandle == -1) {
        return false; // 接続失敗
    }

    isHost = false;
    isConnected = true;
    return true;
}

void NetworkManager::Update() {
    // 1. ホストで、まだ誰も接続してきていない場合の処理
    if (isHost && !isConnected) {
        // 新しい接続要求が来ているかチェック
        int newHandle = GetNewAcceptNetWork();
        if (newHandle != -1) {
            netHandle = newHandle;
            isConnected = true;
            StopListenNetWork(); // 1対1の対戦なので、受付を終了する
        }
    }

    // 2. 接続が確立している場合の受信処理
    if (isConnected) {
        // 相手が通信を切断したかチェック
        if (GetLostNetWork() == netHandle) {
            Disconnect();
            return;
        }

        // 受信データが来ているかチェック
        int dataLength = GetNetWorkDataLength(netHandle);
        if (dataLength >= sizeof(GamePacket)) {
            GamePacket packet;
            NetWorkRecv(netHandle, &packet, sizeof(GamePacket));

            // 受信したらすぐに処理せず、キューに突っ込む
            packetQueue.push(packet);
        }
    }
}

void NetworkManager::SendPacket(const GamePacket& packet) {
    if (isConnected && netHandle != -1) {
        // DxLibの通信関数を使ってデータを送信する
        // NetWorkSend(接続先ハンドル, 送信データのアドレス, サイズ)
        NetWorkSend(netHandle, &packet, sizeof(GamePacket));
    }
}

void NetworkManager::Disconnect() {
    if (netHandle != -1) {
        CloseNetWork(netHandle);
        netHandle = -1;
    }
    if (isHost) {
        StopListenNetWork(); // 待機中なら待機もキャンセル
    }
    isConnected = false;
    isHost = false;
}

bool NetworkManager::PopPacket(GamePacket& outPacket) {
    if (packetQueue.empty()) return false;
    outPacket = packetQueue.front();
    packetQueue.pop();
    return true;
}