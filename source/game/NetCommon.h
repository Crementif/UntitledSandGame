#pragma once
#include "../common/types.h"

typedef uint32_t PlayerID;

enum NET_ACTION : u8
{
    // client to server
    //NET_ACTION_C_START_GAME = 0x01,
    NET_ACTION_C_MOVEMENT = 0x02,
    NET_ACTION_C_ABILITY = 0x03,
    NET_ACTION_C_DRILLING = 0x04,
    NET_ACTION_C_SYNCED_EVENT = 0x05, // e.g. explosions

    // server to client
    NET_ACTION_S_START_GAME = 0x40,
    NET_ACTION_S_MOVEMENT = 0x41,
    NET_ACTION_S_ABILITY = 0x42,
    NET_ACTION_S_DRILLING = 0x43,
    NET_ACTIONS_S_SYNCED_EVENT = 0x44,
};