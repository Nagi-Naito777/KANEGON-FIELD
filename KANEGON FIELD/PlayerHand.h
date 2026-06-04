// プレイヤーの手札関係クラス

#pragma once
#include <vector>
#include <algorithm>
#include "Card.h"

// カードの最大所持枚数
#define CARD_MAX 18

class PlayerHand {
private:
    std::vector<Card> hand;

public:
    PlayerHand() = default;
    ~PlayerHand() = default;

    // 手札にカードを追加(ドロー)
    void Add(const Card& newCard);

    // カードを削除(使用したときなど)
    void Remove(int index);

    // 手札を全て捨てる
    void Clear();

    // 手札を並べ替える
    void Sort();

    // 全手札を取得
    const std::vector<Card>& GetCards() const { return hand; }

    // 手札の枚数を取得
    int GetCount() const { return (int)hand.size(); }
};

