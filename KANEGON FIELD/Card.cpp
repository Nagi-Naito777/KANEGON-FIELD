#include "Card.h"

// デフォルトコンストラクタ
Card::Card() {
    graphicIndex = 0;
    data.power = 0;
    data.name = "なし";
    data.type = "なし";
    data.category = UnNull;
}

// データセット用コンストラクタ
Card::Card(int id, std::string name, int power, std::string type,
    std::string setumei, CardCategory category, bool can_add, int money, int mp, int percent)
{
    data.ID = id;
    data.name = name;
    data.power = power;
    data.type = type;
    data.setumei = setumei;
    data.category = category;
    data.add = can_add;
    data.money = money;
    data.MP = mp;
    data.percent = percent;

    this->graphicIndex = id;
}