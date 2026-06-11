#include "PlayerHand.h"

void PlayerHand::Add(const Card& newCard) {
    if (hand.size() < CARD_MAX) {
        hand.push_back(newCard);
    }
}

void PlayerHand::Remove(int index) {
    if (index >= 0 && index < hand.size()) {
        hand.erase(hand.begin() + index);
    }
}

void PlayerHand::Clear() {
    // クリア関数で簡単かつ安全に消去
    hand.clear();
}

void PlayerHand::Sort() {
    if (hand.empty()) return;

    std::sort(hand.begin(), hand.end(), [](const Card& a, const Card& b) {
        // --- 奇跡カードを末尾に送る ---
        bool aIsMiracle = (a.GetCategory() == CardCategory::Magic);
        bool bIsMiracle = (b.GetCategory() == CardCategory::Magic);

        if (aIsMiracle != bIsMiracle) {
            return bIsMiracle; // bが奇跡ならtrue (aの方が前)、aが奇跡ならfalse (bの方が前)
        }

        // カテゴリで並び替え
        if (a.GetCategory() != b.GetCategory()) {
            return a.GetCategory() < b.GetCategory();
        }

        // 攻撃カードのみ属性枠を後ろにする 
        if (a.GetCategory() == CardCategory::Attack) {
            auto getAttrPriority = [](const std::string& type) {
                if (type == "" || type == "無") return 0;
                if (type == "炎") return 1;
                if (type == "水") return 2;
                if (type == "木") return 3;
                if (type == "光") return 4;
                if (type == "闇") return 5;
                return 6;
                };

            int priorityA = getAttrPriority(a.GetType());
            int priorityB = getAttrPriority(b.GetType());

            if (priorityA != priorityB) {
                return priorityA < priorityB;
            }
        }

        // 威力比較
        if (a.GetPower() != b.GetPower()) {
            return a.GetPower() < b.GetPower();
        }

        // 追加攻撃フラグで比較
        if (a.GetAdd() != b.GetAdd()) {
            return (int)a.GetAdd() < (int)b.GetAdd();
        }

        // 威力が1かつ無属性の場合のみ、金額で比較
        // (ゴッフィーで言うこん棒シリーズ)
        if (a.GetPower() == 1 && a.GetType() == "無" && b.GetType() == "無") {
            return a.GetMoney() < b.GetMoney();
        }

        return false;
        });
}
