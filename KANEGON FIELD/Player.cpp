#include "Player.h"

// コンストラクタ
Player::Player()
    : hp(40), mp(10), money(20), ID(0), team(0), controlType(ControllerType::HUMAN), name("NoName")
{
}

// 引数付きコンストラクタの実装
Player::Player(std::string n, ControllerType type)
    : hp(40), mp(10), money(20), ID(0), team(0), controlType(type), name(n)
{
}

void Player::setHp(int newHp) {
    if (newHp < 0) newHp = 0;
    if (newHp > 99) newHp = 99;
    hp = newHp;
}

void Player::setMp(int newMp) {
    if (newMp < 0) newMp = 0;
    if (newMp > 99) newMp = 99;
    mp = newMp;
}

void Player::setMoney(int newMoney) {
    if (newMoney < 0) newMoney = 0;
    if (newMoney > 99) newMoney = 99;
    money = newMoney;
}

// グローバル変数の実体化
Player g_player;