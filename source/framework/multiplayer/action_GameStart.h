#pragma once
#include "../../common/common.h"
#include "multiplayer.h"


struct action_GameStart : GameAction {
    u32 level;
    u32 spawnIdx;
    u32 playerIdx;
};
