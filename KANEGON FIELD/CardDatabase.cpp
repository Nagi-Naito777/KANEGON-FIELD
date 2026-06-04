#include "CardDatabase.h"
#include <fstream>
#include <sstream>
#include <random>
#include <iostream>

// 文字列からカテゴリへの変換
CardCategory CardDatabase::StringToCategory(const std::string& str) const {
    if (str == "攻") return CardCategory::Attack;
    if (str == "守") return CardCategory::Defense;
    if (str == "奇") return CardCategory::Magic;
    if (str == "癒") return CardCategory::Healing;
    if (str == "魔") return CardCategory::MagicHealing;
    if (str == "買") return CardCategory::Buy;
    if (str == "売") return CardCategory::Sell;
    if (str == "換") return CardCategory::Change;
    if (str == "両") return CardCategory::Bilingual;
    if (str == "全") return CardCategory::All;
    return CardCategory::UnNull;
}

// CSV読み込み
bool CardDatabase::Load(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    std::string line;
    if (!std::getline(file, line)) return false; // 1行目（ヘッダー）を読み飛ばし

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        std::stringstream ss(line);
        std::string cell;
        std::vector<std::string> row;

        while (std::getline(ss, cell, ',')) {
            row.push_back(cell);
        }

        if (row.size() < 10) continue;

        try {
            // 文字列からカテゴリに変換
            CardCategory category = StringToCategory(row[5]);

            // リストにカードを追加
            cardList.emplace_back(
                std::stoi(row[0]), row[1], std::stoi(row[2]),
                row[3], row[4], category,
                (row[6] == "1"),
                std::stoi(row[7]), std::stoi(row[8]), std::stoi(row[9])
            );
        }
        catch (const std::exception& e) {
            std::cerr << "データ変換エラー: " << e.what() << " 行: " << line << std::endl;
        }
    }
    return true;
}

// ランダムなカードを1枚返す
Card CardDatabase::GetRandomCard() const {
    if (cardList.empty()) {
        return Card(); // 空の場合はデフォルトカード
    }

    static std::mt19937 engine(static_cast<unsigned int>(time(nullptr)));
    std::uniform_int_distribution<size_t> dist(0, cardList.size() - 1);

    return cardList[dist(engine)];
}

// IDからカードを検索して返す
Card CardDatabase::GetCardByID(int id) const {
    for (const auto& card : cardList) {
        if (card.GetID() == id) {
            return card;
        }
    }
    return Card(); // 見つからなかったらデフォルトカード
}

// メモリ上に実体を作成
CardDatabase CardDB;