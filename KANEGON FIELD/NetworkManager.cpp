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
    // 1. ホストの場合：常に接続待機を続ける
    if (isHost) {
        // 新しい接続要求が来ているかチェック
        int newHandle = GetNewAcceptNetWork();
        if (newHandle != -1) {
            clientHandles.push_back(newHandle); // 新しい人をリストに追加
            isConnected = true; // 誰か一人でもいれば connected 扱い
        }

        // 切断チェック
        int lostHandle = GetLostNetWork();
        if (lostHandle != -1) {
            // リストから削除
            for (auto it = clientHandles.begin(); it != clientHandles.end(); ) {
                if (*it == lostHandle) {
                    CloseNetWork(*it); // ★確実に閉じる
                    it = clientHandles.erase(it);
                }
                else {
                    ++it;
                }
            }
            // 誰もいなくなったら isConnected を false にするなどの処理を入れてもOK
            if (clientHandles.empty()) {
                isConnected = false;
            }
        }
    }
    // クライアント側の場合、ホストが落ちた（切断された）かチェック
    else if (isConnected) {
        if (GetLostNetWork() == netHandle) {
            Disconnect(); // 切断処理
            return;       // これ以上処理しない
        }
    }

    // 2. 受信処理（ホストとクライアントで分ける）
    if (isHost) {
        // ホスト：全員分を受信してキューに入れる
        for (int handle : clientHandles) {
            while (GetNetWorkDataLength(handle) >= sizeof(GamePacket)) {
                GamePacket packet;
                if (NetWorkRecv(handle, &packet, sizeof(GamePacket)) >= 0) {
                    packetQueue.push(packet);
                }
                else { break; }
            }
        }
    }
    else {
        // クライアント：netHandle のみを受信
        if (isConnected && netHandle != -1) {
            while (GetNetWorkDataLength(netHandle) >= sizeof(GamePacket)) {
                GamePacket packet;
                if (NetWorkRecv(netHandle, &packet, sizeof(GamePacket)) >= 0) {
                    packetQueue.push(packet);
                }
                else { break; }
            }
        }
    }
}

// 全員にパケットを送る（ブロードキャスト）
void NetworkManager::BroadcastPacket(const GamePacket& packet) {
    for (int handle : clientHandles) {
        NetWorkSend(handle, &packet, sizeof(GamePacket));
    }
}

void NetworkManager::SendPacket(const GamePacket& packet) {
    if (isConnected && netHandle != -1) {
        // DxLibの通信関数を使ってデータを送信する
        // NetWorkSend(接続先ハンドル, 送信データのアドレス, サイズ)
        NetWorkSend(netHandle, &packet, sizeof(GamePacket));
    }
}

void NetworkManager::SendPacketTo(int handle, const GamePacket& packet) {
    if (handle != -1) {
        NetWorkSend(handle, &packet, sizeof(GamePacket));
    }
}

void NetworkManager::Disconnect() {
    // クライアント用ハンドルの破棄
    if (netHandle != -1) {
        CloseNetWork(netHandle);
        netHandle = -1;
    }

    // ホスト用：全クライアントの破棄
    for (int handle : clientHandles) {
        CloseNetWork(handle);
    }
    clientHandles.clear();

    if (isHost) {
        StopListenNetWork();
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