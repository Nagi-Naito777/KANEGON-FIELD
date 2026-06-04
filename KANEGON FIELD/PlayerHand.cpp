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
    // ƒNƒŠƒAŠÖ”‚ÅŠÈ’P‚©‚ÂˆÀ‘S‚ÉÁ‹Ž
    hand.clear();
}

void PlayerHand::Sort() {
    if (hand.empty()) return;

    std::sort(hand.begin(), hand.end(), [](const Card& a, const Card& b) {
        // ƒJƒeƒSƒŠ‚Å•À‚Ñ‘Ö‚¦
        if (a.GetCategory() != b.GetCategory()) {
            return a.GetCategory() < b.GetCategory();
        }

        // UŒ‚ƒJ[ƒh‚Ì‚Ý‘®«˜g‚ðŒã‚ë‚É‚·‚é 
        if (a.GetCategory() == CardCategory::Attack) {
            auto getAttrPriority = [](const std::string& type) {
                if (type == "" || type == "–³") return 0;
                if (type == "‰Š") return 1;
                if (type == "…") return 2;
                if (type == "–Ø") return 3;
                if (type == "Œõ") return 4;
                if (type == "ˆÅ") return 5;
                return 6;
                };

            int priorityA = getAttrPriority(a.GetType());
            int priorityB = getAttrPriority(b.GetType());

            if (priorityA != priorityB) {
                return priorityA < priorityB;
            }
        }

        // ˆÐ—Í”äŠr
        if (a.GetPower() != b.GetPower()) {
            return a.GetPower() < b.GetPower();
        }

        // ’Ç‰ÁUŒ‚ƒtƒ‰ƒO‚Å”äŠr
        if (a.GetAdd() != b.GetAdd()) {
            return (int)a.GetAdd() < (int)b.GetAdd();
        }

        // ˆÐ—Í‚ª1‚©‚Â–³‘®«‚Ìê‡‚Ì‚ÝA‹àŠz‚Å”äŠr
        // (ƒSƒbƒtƒB[‚ÅŒ¾‚¤‚±‚ñ–_ƒVƒŠ[ƒY)
        if (a.GetPower() == 1 && a.GetType() == "–³" && b.GetType() == "–³") {
            return a.GetMoney() < b.GetMoney();
        }

        return false;
        });
}
