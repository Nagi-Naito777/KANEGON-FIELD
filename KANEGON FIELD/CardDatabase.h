// カードデータベースクラス
#pragma once
#include "Card.h"
#include <vector>
#include <string>

class CardDatabase {
private:
    std::vector<Card> cardList; // 読み込んだ全カードを保存するリスト

    // 文字列を列挙体に変換する補助関数
    CardCategory StringToCategory(const std::string& str) const;

public:
    CardDatabase() = default;
    ~CardDatabase() = default;

    // CSVからカード情報を読み込む
    bool Load(const std::string& filePath);

    // データベースからランダムに1枚取得する（配る用）
    Card GetRandomCard() const;

    // （おまけ）IDを指定して特定のカードを取得する関数（絶対後で必要になります）
    Card GetCardByID(int id) const;
};

// どこからでもデータベースにアクセスできるようにする
extern CardDatabase CardDB;

