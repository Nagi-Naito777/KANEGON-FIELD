#pragma once
#include "BattleData.h"

class BattleAIManager
{
public:
    // AIの行動を決定・実行する関数
    // humanIdx: プレイヤー（人間）のインデックス
    // isHumanTurn: 現在人間のターンかどうか
    void Update(BattleData& data, int humanIdx, bool isHumanTurn);
};

